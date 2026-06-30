#include "postgres.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PostgresLib pg_lib = {0};

BOOL postgres_init(void) {
  static BOOL alert_shown = FALSE;
  if (pg_lib.hLib)
    return TRUE;

  pg_lib.hLib = LoadLibraryA("libpq.dll");
  if (!pg_lib.hLib) {
    if (!alert_shown) {
      DWORD err = GetLastError();
      char msg[1024];
      snprintf(msg, sizeof(msg),
               "Failed to load libpq.dll (Error Code: %lu).\n\n"
               "Possible reasons:\n"
               "1. Missing dependency DLLs (like libcrypto-3-x64-*.dll, libssl-3-x64-*.dll, etc.)\n"
               "2. Missing Visual C++ Redistributable (VCRedist)\n"
               "3. Bitness mismatch (e.g. 64-bit DLL with 32-bit executable)\n\n"
               "Please ensure all required DLLs are in the same folder as dbmrap.exe.",
               err);
      MessageBoxA(NULL, msg, "PostgreSQL Driver Error", MB_OK | MB_ICONERROR);
      alert_shown = TRUE;
    }
    return FALSE;
  }

  pg_lib.connectdb = (fn_PQconnectdb)GetProcAddress(pg_lib.hLib, "PQconnectdb");
  pg_lib.status = (fn_PQstatus)GetProcAddress(pg_lib.hLib, "PQstatus");
  pg_lib.errorMessage =
      (fn_PQerrorMessage)GetProcAddress(pg_lib.hLib, "PQerrorMessage");
  pg_lib.finish = (fn_PQfinish)GetProcAddress(pg_lib.hLib, "PQfinish");
  pg_lib.exec = (fn_PQexec)GetProcAddress(pg_lib.hLib, "PQexec");
  pg_lib.resultStatus =
      (fn_PQresultStatus)GetProcAddress(pg_lib.hLib, "PQresultStatus");
  pg_lib.ntuples = (fn_PQntuples)GetProcAddress(pg_lib.hLib, "PQntuples");
  pg_lib.nfields = (fn_PQnfields)GetProcAddress(pg_lib.hLib, "PQnfields");
  pg_lib.fname = (fn_PQfname)GetProcAddress(pg_lib.hLib, "PQfname");
  pg_lib.getvalue = (fn_PQgetvalue)GetProcAddress(pg_lib.hLib, "PQgetvalue");
  pg_lib.getisnull = (fn_PQgetisnull)GetProcAddress(pg_lib.hLib, "PQgetisnull");
  pg_lib.cmdTuples = (fn_PQcmdTuples)GetProcAddress(pg_lib.hLib, "PQcmdTuples");
  pg_lib.cmdStatus = (fn_PQcmdStatus)GetProcAddress(pg_lib.hLib, "PQcmdStatus");
  pg_lib.clear = (fn_PQclear)GetProcAddress(pg_lib.hLib, "PQclear");

  if (!pg_lib.connectdb || !pg_lib.status || !pg_lib.errorMessage ||
      !pg_lib.finish || !pg_lib.exec || !pg_lib.resultStatus ||
      !pg_lib.ntuples || !pg_lib.nfields || !pg_lib.getvalue ||
      !pg_lib.cmdStatus || !pg_lib.clear) {
    FreeLibrary(pg_lib.hLib);
    memset(&pg_lib, 0, sizeof(PostgresLib));
    return FALSE;
  }

  return TRUE;
}

void postgres_cleanup(void) {
  if (pg_lib.hLib) {
    FreeLibrary(pg_lib.hLib);
    memset(&pg_lib, 0, sizeof(PostgresLib));
  }
}

static PGconn *pg_connect_db(ConnectionInfo *info, const char *db_name) {
  if (!postgres_init())
    return NULL;

  const char *db = db_name;
  if (!db || strlen(db) == 0) {
    db = (info->database && strlen(info->database) > 0) ? info->database
                                                        : "postgres";
  }
  char conninfo[512];
  snprintf(conninfo, sizeof(conninfo),
           "host=%s port=%s user=%s password=%s dbname=%s", info->host,
           info->port, info->username, info->password, db);
  return pg_lib.connectdb(conninfo);
}

static void pg_load_columns(PGconn *conn, DbTreeCallback cb, void *user_data,
                            void *hTableItem, const char *schema,
                            const char *table, const char *conn_id,
                            const char *db_name) {
  void *hColParent = cb(user_data, hTableItem, NODE_OTHER, "Columns", conn_id,
                        db_name, schema, table);

  char query[512];
  snprintf(query, sizeof(query),
           "SELECT column_name, data_type, character_maximum_length, "
           "numeric_precision "
           "FROM information_schema.columns "
           "WHERE table_schema='%s' AND table_name='%s' "
           "ORDER BY ordinal_position;",
           schema, table);

  PGresult *res = pg_lib.exec(conn, query);
  if (pg_lib.resultStatus(res) == PGRES_TUPLES_OK) {
    for (int i = 0; i < pg_lib.ntuples(res); i++) {
      const char *col_name = pg_lib.getvalue(res, i, 0);
      const char *data_type = pg_lib.getvalue(res, i, 1);
      const char *char_len = pg_lib.getvalue(res, i, 2);
      const char *num_prec = pg_lib.getvalue(res, i, 3);

      char col_display[256];
      if (char_len && strlen(char_len) > 0) {
        snprintf(col_display, sizeof(col_display), "%s (%s(%s))", col_name,
                 data_type, char_len);
      } else if (num_prec && strlen(num_prec) > 0) {
        snprintf(col_display, sizeof(col_display), "%s (%s(%s))", col_name,
                 data_type, num_prec);
      } else {
        snprintf(col_display, sizeof(col_display), "%s (%s)", col_name,
                 data_type);
      }

      cb(user_data, hColParent, NODE_COLUMN, col_display, conn_id, db_name,
         schema, table);
    }
  }
  pg_lib.clear(res);
}

static void pg_load_foreign_keys(PGconn *conn, DbTreeCallback cb,
                                 void *user_data, void *hTableItem,
                                 const char *schema, const char *table,
                                 const char *conn_id, const char *db_name) {
  void *hFkParent = cb(user_data, hTableItem, NODE_OTHER, "Foreign Keys",
                       conn_id, db_name, schema, table);

  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT "
           "    kcu.constraint_name, "
           "    kcu.column_name, "
           "    ccu.table_schema AS ref_schema, "
           "    ccu.table_name AS ref_table, "
           "    ccu.column_name AS ref_column "
           "FROM information_schema.table_constraints tc "
           "JOIN information_schema.key_column_usage kcu "
           "    ON tc.constraint_name = kcu.constraint_name "
           "    AND tc.table_schema = kcu.table_schema "
           "JOIN information_schema.constraint_column_usage ccu "
           "    ON ccu.constraint_name = tc.constraint_name "
           "WHERE tc.constraint_type = 'FOREIGN KEY' "
           "    AND tc.table_schema = '%s' "
           "    AND tc.table_name = '%s';",
           schema, table);

  PGresult *res = pg_lib.exec(conn, query);
  if (pg_lib.resultStatus(res) == PGRES_TUPLES_OK) {
    for (int i = 0; i < pg_lib.ntuples(res); i++) {
      const char *fk_name = pg_lib.getvalue(res, i, 0);
      const char *col_name = pg_lib.getvalue(res, i, 1);
      const char *ref_schema = pg_lib.getvalue(res, i, 2);
      const char *ref_table = pg_lib.getvalue(res, i, 3);
      const char *ref_col = pg_lib.getvalue(res, i, 4);

      char fk_display[512];
      snprintf(fk_display, sizeof(fk_display), "%s: %s -> %s.%s.%s", fk_name,
               col_name, ref_schema, ref_table, ref_col);

      cb(user_data, hFkParent, NODE_OTHER, fk_display, conn_id, db_name, schema,
         table);
    }
  }
  pg_lib.clear(res);
}

static void pg_load_tables(PGconn *conn, DbTreeCallback cb, void *user_data,
                           void *hSchemaItem, const char *schema,
                           const char *conn_id, const char *db_name) {
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT table_name FROM information_schema.tables "
           "WHERE table_schema='%s' AND table_type='BASE TABLE';",
           schema);

  PGresult *res = pg_lib.exec(conn, query);
  if (pg_lib.resultStatus(res) == PGRES_TUPLES_OK) {
    for (int t = 0; t < pg_lib.ntuples(res); t++) {
      const char *table_name = pg_lib.getvalue(res, t, 0);

      void *hTableItem = cb(user_data, hSchemaItem, NODE_TABLE, table_name,
                            conn_id, db_name, schema, table_name);
      pg_load_columns(conn, cb, user_data, hTableItem, schema, table_name,
                      conn_id, db_name);
      pg_load_foreign_keys(conn, cb, user_data, hTableItem, schema, table_name,
                           conn_id, db_name);
    }
  }
  pg_lib.clear(res);
}

static void pg_load_schemas(PGconn *conn, DbTreeCallback cb, void *user_data,
                            void *hDbItem, const char *conn_id,
                            const char *db_name) {
  PGresult *res =
      pg_lib.exec(conn, "SELECT schema_name FROM information_schema.schemata "
                        "WHERE schema_name NOT LIKE 'pg_%' "
                        "AND schema_name != 'information_schema';");

  if (pg_lib.resultStatus(res) == PGRES_TUPLES_OK) {
    for (int s = 0; s < pg_lib.ntuples(res); s++) {
      const char *schema_name = pg_lib.getvalue(res, s, 0);

      void *hSchemaItem = cb(user_data, hDbItem, NODE_SCHEMA, schema_name,
                             conn_id, db_name, schema_name, "");
      pg_load_tables(conn, cb, user_data, hSchemaItem, schema_name, conn_id,
                     db_name);
    }
  }
  pg_lib.clear(res);
}

void pg_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                  void *conn_handle) {
  if (!postgres_init()) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to load libpq.dll", info->id,
       "", "", "");
    return;
  }

  const char *initial_db = (info->database && strlen(info->database) > 0)
                               ? info->database
                               : "postgres";
  PGconn *conn = pg_connect_db(info, initial_db);
  if (!conn || pg_lib.status(conn) != CONNECTION_OK) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to connect", info->id, "",
       "", "");
    if (conn)
      pg_lib.finish(conn);
    return;
  }

  PGresult *res;
  if (!info->database || strlen(info->database) == 0) {
    res = pg_lib.exec(
        conn, "SELECT datname FROM pg_database WHERE datistemplate = false;");
  } else {
    res = pg_lib.exec(conn, "SELECT current_database();");
  }

  if (pg_lib.resultStatus(res) == PGRES_TUPLES_OK) {
    int rows = pg_lib.ntuples(res);
    for (int i = 0; i < rows; i++) {
      const char *db = pg_lib.getvalue(res, i, 0);
      void *hDbItem =
          cb(user_data, conn_handle, NODE_DATABASE, db, info->id, db, "", "");

      PGconn *db_conn = pg_connect_db(info, db);
      if (db_conn && pg_lib.status(db_conn) == CONNECTION_OK) {
        pg_load_schemas(db_conn, cb, user_data, hDbItem, info->id, db);
      } else {
        cb(user_data, hDbItem, NODE_OTHER, "Failed to connect to database",
           info->id, db, "", "");
      }
      if (db_conn)
        pg_lib.finish(db_conn);
    }
  }
  pg_lib.clear(res);
  pg_lib.finish(conn);
}

void pg_run_query(ConnectionInfo *info, const char *active_db,
                  const char *query, DbHeaderCallback header_cb,
                  DbRowCallback row_cb, DbStatusCallback status_cb,
                  void *user_data) {
  if (!postgres_init()) {
    status_cb(user_data, "Failed to load libpq.dll. Make sure it is in the "
                         "executable directory or path.");
    return;
  }

  PGconn *conn = pg_connect_db(info, active_db);
  if (!conn || pg_lib.status(conn) != CONNECTION_OK) {
    status_cb(user_data,
              conn ? pg_lib.errorMessage(conn) : "Failed to connect");
    if (conn)
      pg_lib.finish(conn);
    return;
  }

  PGresult *res = pg_lib.exec(conn, query);
  ExecStatusType status = pg_lib.resultStatus(res);

  if (status == PGRES_TUPLES_OK) {
    int cols = pg_lib.nfields(res);
    for (int c = 0; c < cols; c++) {
      header_cb(user_data, c, pg_lib.fname(res, c));
    }

    int rows = pg_lib.ntuples(res);
    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
        row_cb(user_data, r, c, pg_lib.getvalue(res, r, c));
      }
    }

    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg), "OK: %d row(s)", rows);
    status_cb(user_data, status_msg);
  } else if (status == PGRES_COMMAND_OK) {
    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg),
             "OK: Command completed successfully. Command status: %s",
             pg_lib.cmdStatus(res));
    status_cb(user_data, status_msg);
  } else {
    status_cb(user_data, pg_lib.errorMessage(conn));
  }

  pg_lib.clear(res);
  pg_lib.finish(conn);
}
