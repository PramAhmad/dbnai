#include "postgres.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PGconn *pg_connect_db(ConnectionInfo *info, const char *db_name) {
  const char *db = db_name;
  if (!db || strlen(db) == 0) {
    db = (info->database && strlen(info->database) > 0) ? info->database
                                                        : "postgres";
  }
  char conninfo[512];
  snprintf(conninfo, sizeof(conninfo),
           "host=%s port=%s user=%s password=%s dbname=%s", info->host,
           info->port, info->username, info->password, db);
  return PQconnectdb(conninfo);
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

  PGresult *res = PQexec(conn, query);
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    for (int i = 0; i < PQntuples(res); i++) {
      const char *col_name = PQgetvalue(res, i, 0);
      const char *data_type = PQgetvalue(res, i, 1);
      const char *char_len = PQgetvalue(res, i, 2);
      const char *num_prec = PQgetvalue(res, i, 3);

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
  PQclear(res);
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

  PGresult *res = PQexec(conn, query);
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    for (int i = 0; i < PQntuples(res); i++) {
      const char *fk_name = PQgetvalue(res, i, 0);
      const char *col_name = PQgetvalue(res, i, 1);
      const char *ref_schema = PQgetvalue(res, i, 2);
      const char *ref_table = PQgetvalue(res, i, 3);
      const char *ref_col = PQgetvalue(res, i, 4);

      char fk_display[512];
      snprintf(fk_display, sizeof(fk_display), "%s: %s -> %s.%s.%s", fk_name,
               col_name, ref_schema, ref_table, ref_col);

      cb(user_data, hFkParent, NODE_OTHER, fk_display, conn_id, db_name, schema,
         table);
    }
  }
  PQclear(res);
}

static void pg_load_tables(PGconn *conn, DbTreeCallback cb, void *user_data,
                           void *hSchemaItem, const char *schema,
                           const char *conn_id, const char *db_name) {
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT table_name FROM information_schema.tables "
           "WHERE table_schema='%s' AND table_type='BASE TABLE';",
           schema);

  PGresult *res = PQexec(conn, query);
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    for (int t = 0; t < PQntuples(res); t++) {
      const char *table_name = PQgetvalue(res, t, 0);

      void *hTableItem = cb(user_data, hSchemaItem, NODE_TABLE, table_name,
                            conn_id, db_name, schema, table_name);
      pg_load_columns(conn, cb, user_data, hTableItem, schema, table_name,
                      conn_id, db_name);
      pg_load_foreign_keys(conn, cb, user_data, hTableItem, schema, table_name,
                           conn_id, db_name);
    }
  }
  PQclear(res);
}

static void pg_load_schemas(PGconn *conn, DbTreeCallback cb, void *user_data,
                            void *hDbItem, const char *conn_id,
                            const char *db_name) {
  PGresult *res =
      PQexec(conn, "SELECT schema_name FROM information_schema.schemata "
                   "WHERE schema_name NOT LIKE 'pg_%' "
                   "AND schema_name != 'information_schema';");

  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    for (int s = 0; s < PQntuples(res); s++) {
      const char *schema_name = PQgetvalue(res, s, 0);

      void *hSchemaItem = cb(user_data, hDbItem, NODE_SCHEMA, schema_name,
                             conn_id, db_name, schema_name, "");
      pg_load_tables(conn, cb, user_data, hSchemaItem, schema_name, conn_id,
                     db_name);
    }
  }
  PQclear(res);
}

void pg_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                  void *conn_handle) {
  const char *initial_db = (info->database && strlen(info->database) > 0)
                               ? info->database
                               : "postgres";
  PGconn *conn = pg_connect_db(info, initial_db);
  if (PQstatus(conn) != CONNECTION_OK) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to connect", info->id, "",
       "", "");
    PQfinish(conn);
    return;
  }

  PGresult *res;
  if (!info->database || strlen(info->database) == 0) {
    res = PQexec(
        conn, "SELECT datname FROM pg_database WHERE datistemplate = false;");
  } else {
    res = PQexec(conn, "SELECT current_database();");
  }

  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
      const char *db = PQgetvalue(res, i, 0);
      void *hDbItem =
          cb(user_data, conn_handle, NODE_DATABASE, db, info->id, db, "", "");

      PGconn *db_conn = pg_connect_db(info, db);
      if (PQstatus(db_conn) == CONNECTION_OK) {
        pg_load_schemas(db_conn, cb, user_data, hDbItem, info->id, db);
      } else {
        cb(user_data, hDbItem, NODE_OTHER, "Failed to connect to database",
           info->id, db, "", "");
      }
      PQfinish(db_conn);
    }
  }
  PQclear(res);
  PQfinish(conn);
}

void pg_run_query(ConnectionInfo *info, const char *active_db,
                  const char *query, DbHeaderCallback header_cb,
                  DbRowCallback row_cb, DbStatusCallback status_cb,
                  void *user_data) {
  PGconn *conn = pg_connect_db(info, active_db);
  if (PQstatus(conn) != CONNECTION_OK) {
    status_cb(user_data, PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  PGresult *res = PQexec(conn, query);
  ExecStatusType status = PQresultStatus(res);

  if (status == PGRES_TUPLES_OK) {
    int cols = PQnfields(res);
    for (int c = 0; c < cols; c++) {
      header_cb(user_data, c, PQfname(res, c));
    }

    int rows = PQntuples(res);
    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
        row_cb(user_data, r, c, PQgetvalue(res, r, c));
      }
    }

    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg), "OK: %d row(s)", rows);
    status_cb(user_data, status_msg);
  } else if (status == PGRES_COMMAND_OK) {
    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg),
             "OK: Command completed successfully. Command status: %s",
             PQcmdStatus(res));
    status_cb(user_data, status_msg);
  } else {
    status_cb(user_data, PQresultErrorMessage(res));
  }

  PQclear(res);
  PQfinish(conn);
}
