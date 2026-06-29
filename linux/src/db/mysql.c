#include "mysql.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MYSQL *mysql_connect_db(ConnectionInfo *info, const char *db_name) {
  MYSQL *conn = mysql_init(NULL);
  if (!conn)
    return NULL;

  unsigned int port_num =
      (info->port && strlen(info->port) > 0) ? atoi(info->port) : 3306;
  const char *db = db_name;
  if (!db || strlen(db) == 0) {
    db = (info->database && strlen(info->database) > 0) ? info->database
                                                        : "mysql";
  }

  if (!mysql_real_connect(conn, info->host, info->username, info->password, db,
                          port_num, NULL, 0)) {
    mysql_close(conn);
    return NULL;
  }
  return conn;
}

static void mysql_load_columns(MYSQL *conn, DbTreeCallback cb, void *user_data,
                               void *hTableItem, const char *db_name,
                               const char *table_name, const char *conn_id) {
  void *hColParent = cb(user_data, hTableItem, NODE_OTHER, "Columns", conn_id,
                        db_name, "", table_name);

  char query[512];
  snprintf(query, sizeof(query),
           "SELECT COLUMN_NAME, COLUMN_TYPE, CHARACTER_MAXIMUM_LENGTH, "
           "NUMERIC_PRECISION "
           "FROM INFORMATION_SCHEMA.COLUMNS "
           "WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='%s' "
           "ORDER BY ORDINAL_POSITION;",
           db_name, table_name);

  if (mysql_query(conn, query) != 0)
    return;

  MYSQL_RES *result = mysql_store_result(conn);
  if (!result)
    return;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != NULL) {
    const char *col_name = row[0];
    const char *data_type = row[1];
    const char *char_len = row[2] ? row[2] : "";
    const char *num_prec = row[3] ? row[3] : "";

    char col_display[256];
    if (strlen(char_len) > 0) {
      snprintf(col_display, sizeof(col_display), "%s (%s(%s))", col_name,
               data_type, char_len);
    } else if (strlen(num_prec) > 0) {
      snprintf(col_display, sizeof(col_display), "%s (%s(%s))", col_name,
               data_type, num_prec);
    } else {
      snprintf(col_display, sizeof(col_display), "%s (%s)", col_name,
               data_type);
    }

    cb(user_data, hColParent, NODE_COLUMN, col_display, conn_id, db_name, "",
       table_name);
  }
  mysql_free_result(result);
}

static void mysql_load_foreign_keys(MYSQL *conn, DbTreeCallback cb,
                                    void *user_data, void *hTableItem,
                                    const char *db_name, const char *table_name,
                                    const char *conn_id) {
  void *hFkParent = cb(user_data, hTableItem, NODE_OTHER, "Foreign Keys",
                       conn_id, db_name, "", table_name);

  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT CONSTRAINT_NAME, COLUMN_NAME, REFERENCED_TABLE_SCHEMA, "
           "REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME "
           "FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE "
           "WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='%s' "
           "AND REFERENCED_TABLE_NAME IS NOT NULL;",
           db_name, table_name);

  if (mysql_query(conn, query) != 0)
    return;

  MYSQL_RES *result = mysql_store_result(conn);
  if (!result)
    return;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != NULL) {
    const char *fk_name = row[0];
    const char *col_name = row[1];
    const char *ref_schema = row[2];
    const char *ref_table = row[3];
    const char *ref_col = row[4];

    char fk_display[512];
    snprintf(fk_display, sizeof(fk_display), "%s: %s -> %s.%s.%s", fk_name,
             col_name, ref_schema, ref_table, ref_col);

    cb(user_data, hFkParent, NODE_OTHER, fk_display, conn_id, db_name, "",
       table_name);
  }
  mysql_free_result(result);
}

static void mysql_load_tables(MYSQL *conn, DbTreeCallback cb, void *user_data,
                              void *hDbItem, const char *db_name,
                              const char *conn_id) {
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES "
           "WHERE TABLE_SCHEMA='%s' AND TABLE_TYPE='BASE TABLE';",
           db_name);

  if (mysql_query(conn, query) != 0)
    return;

  MYSQL_RES *result = mysql_store_result(conn);
  if (!result)
    return;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != NULL) {
    const char *table_name = row[0];

    void *hTableItem = cb(user_data, hDbItem, NODE_TABLE, table_name, conn_id,
                          db_name, "", table_name);
    mysql_load_columns(conn, cb, user_data, hTableItem, db_name, table_name,
                       conn_id);
    mysql_load_foreign_keys(conn, cb, user_data, hTableItem, db_name,
                            table_name, conn_id);
  }
  mysql_free_result(result);
}

void mysql_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                     void *conn_handle) {
  const char *initial_db =
      (info->database && strlen(info->database) > 0) ? info->database : "mysql";
  MYSQL *conn = mysql_connect_db(info, initial_db);
  if (!conn) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to connect", info->id, "",
       "", "");
    return;
  }

  char query_str[256];
  if (!info->database || strlen(info->database) == 0) {
    snprintf(query_str, sizeof(query_str), "SHOW DATABASES;");
  } else {
    snprintf(query_str, sizeof(query_str), "SELECT DATABASE();");
  }

  if (mysql_query(conn, query_str) != 0) {
    mysql_close(conn);
    return;
  }

  MYSQL_RES *result = mysql_store_result(conn);
  if (!result) {
    mysql_close(conn);
    return;
  }

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != NULL) {
    const char *db = row[0];

    // Skip system databases if loading all
    if (!info->database || strlen(info->database) == 0) {
      if (strcmp(db, "mysql") == 0 || strcmp(db, "information_schema") == 0 ||
          strcmp(db, "performance_schema") == 0 || strcmp(db, "sys") == 0) {
        continue;
      }
    }

    void *hDbItem =
        cb(user_data, conn_handle, NODE_DATABASE, db, info->id, db, "", "");
    MYSQL *db_conn = mysql_connect_db(info, db);
    if (db_conn) {
      mysql_load_tables(db_conn, cb, user_data, hDbItem, db, info->id);
      mysql_close(db_conn);
    } else {
      cb(user_data, hDbItem, NODE_OTHER, "Failed to connect to database",
         info->id, db, "", "");
    }
  }
  mysql_free_result(result);
  mysql_close(conn);
}

void mysql_run_query(ConnectionInfo *info, const char *active_db,
                     const char *query, DbHeaderCallback header_cb,
                     DbRowCallback row_cb, DbStatusCallback status_cb,
                     void *user_data) {
  MYSQL *conn = mysql_connect_db(info, active_db);
  if (!conn) {
    status_cb(user_data, "Failed to connect to MySQL database.");
    return;
  }

  if (mysql_query(conn, query) != 0) {
    status_cb(user_data, mysql_error(conn));
    mysql_close(conn);
    return;
  }

  MYSQL_RES *result = mysql_store_result(conn);
  if (!result) {
    // Query didn't return rows (e.g. UPDATE, INSERT)
    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg), "OK: %lld row(s) affected",
             mysql_affected_rows(conn));
    status_cb(user_data, status_msg);
    mysql_close(conn);
    return;
  }

  int cols = mysql_num_fields(result);
  MYSQL_FIELD *fields = mysql_fetch_fields(result);
  for (int c = 0; c < cols; c++) {
    header_cb(user_data, c, fields[c].name);
  }

  int r = 0;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != NULL) {
    for (int c = 0; c < cols; c++) {
      row_cb(user_data, r, c, row[c] ? row[c] : "NULL");
    }
    r++;
  }

  char status_msg[128];
  snprintf(status_msg, sizeof(status_msg), "OK: %d row(s)", r);
  status_cb(user_data, status_msg);

  mysql_free_result(result);
  mysql_close(conn);
}
