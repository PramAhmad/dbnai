#include "sqlite.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sqlite_load_columns(sqlite3 *db, DbTreeCallback cb, void *user_data,
                                void *hTableItem, const char *table_name,
                                const char *conn_id) {
  void *hColParent = cb(user_data, hTableItem, NODE_OTHER, "Columns", conn_id,
                        "", "", table_name);

  char query[256];
  snprintf(query, sizeof(query), "PRAGMA table_info(%s);", table_name);

  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *col_name = (const char *)sqlite3_column_text(stmt, 1);
      const char *col_type = (const char *)sqlite3_column_text(stmt, 2);

      char display[256];
      snprintf(display, sizeof(display), "%s (%s)", col_name, col_type);
      cb(user_data, hColParent, NODE_COLUMN, display, conn_id, "", "",
         table_name);
    }
    sqlite3_finalize(stmt);
  }
}

static void sqlite_load_foreign_keys(sqlite3 *db, DbTreeCallback cb,
                                     void *user_data, void *hTableItem,
                                     const char *table_name,
                                     const char *conn_id) {
  void *hFkParent = cb(user_data, hTableItem, NODE_OTHER, "Foreign Keys",
                       conn_id, "", "", table_name);

  char query[256];
  snprintf(query, sizeof(query), "PRAGMA foreign_key_list(%s);", table_name);

  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *ref_table = (const char *)sqlite3_column_text(stmt, 2);
      const char *from_col = (const char *)sqlite3_column_text(stmt, 3);
      const char *to_col = (const char *)sqlite3_column_text(stmt, 4);

      char display[512];
      snprintf(display, sizeof(display), "%s -> %s.%s", from_col, ref_table,
               to_col);
      cb(user_data, hFkParent, NODE_OTHER, display, conn_id, "", "",
         table_name);
    }
    sqlite3_finalize(stmt);
  }
}

void sqlite_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                      void *conn_handle) {
  sqlite3 *db = NULL;
  if (sqlite3_open(info->database, &db) != SQLITE_OK) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to open SQLite file",
       info->id, "", "", "");
    if (db)
      sqlite3_close(db);
    return;
  }

  void *hDbItem = cb(user_data, conn_handle, NODE_DATABASE, "main", info->id,
                     "main", "", "");

  sqlite3_stmt *stmt = NULL;
  const char *query = "SELECT name FROM sqlite_master WHERE type='table' AND "
                      "name NOT LIKE 'sqlite_%';";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *table_name = (const char *)sqlite3_column_text(stmt, 0);
      void *hTableItem = cb(user_data, hDbItem, NODE_TABLE, table_name,
                            info->id, "main", "", table_name);
      sqlite_load_columns(db, cb, user_data, hTableItem, table_name, info->id);
      sqlite_load_foreign_keys(db, cb, user_data, hTableItem, table_name,
                               info->id);
    }
    sqlite3_finalize(stmt);
  }

  sqlite3_close(db);
}

void sqlite_run_query(ConnectionInfo *info, const char *active_db,
                      const char *query, DbHeaderCallback header_cb,
                      DbRowCallback row_cb, DbStatusCallback status_cb,
                      void *user_data) {
  (void)active_db;
  sqlite3 *db = NULL;
  if (sqlite3_open(info->database, &db) != SQLITE_OK) {
    status_cb(user_data,
              db ? sqlite3_errmsg(db) : "Failed to open SQLite database.");
    if (db)
      sqlite3_close(db);
    return;
  }

  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    status_cb(user_data, sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  int cols = sqlite3_column_count(stmt);
  for (int c = 0; c < cols; c++) {
    header_cb(user_data, c, sqlite3_column_name(stmt, c));
  }

  int r = 0;
  int step_res;
  while ((step_res = sqlite3_step(stmt)) == SQLITE_ROW) {
    for (int c = 0; c < cols; c++) {
      const char *val = (const char *)sqlite3_column_text(stmt, c);
      row_cb(user_data, r, c, val ? val : "NULL");
    }
    r++;
  }

  char status_msg[128];
  snprintf(status_msg, sizeof(status_msg), "OK: %d row(s)", r);
  status_cb(user_data, status_msg);

  sqlite3_finalize(stmt);
  sqlite3_close(db);
}
