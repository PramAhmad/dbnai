#include "sqlite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SqliteLib sqlite_lib = {0};

BOOL sqlite_init_lib(void) {
  static BOOL alert_shown = FALSE;
  if (sqlite_lib.hLib)
    return TRUE;

  sqlite_lib.hLib = LoadLibraryA("sqlite3.dll");
  if (!sqlite_lib.hLib) {
    if (!alert_shown) {
      DWORD err = GetLastError();
      char msg[1024];
      snprintf(msg, sizeof(msg),
               "Failed to load sqlite3.dll (Error Code: %lu).\n\n"
               "Possible reasons:\n"
               "1. Missing Visual C++ Redistributable (VCRedist)\n"
               "2. Bitness mismatch (e.g. 64-bit DLL with 32-bit executable)\n\n"
               "Please ensure sqlite3.dll is in the same folder as dbmrap.exe.",
               err);
      MessageBoxA(NULL, msg, "SQLite Driver Error", MB_OK | MB_ICONERROR);
      alert_shown = TRUE;
    }
    return FALSE;
  }

  sqlite_lib.open =
      (fn_sqlite3_open)GetProcAddress(sqlite_lib.hLib, "sqlite3_open");
  sqlite_lib.close =
      (fn_sqlite3_close)GetProcAddress(sqlite_lib.hLib, "sqlite3_close");
  sqlite_lib.prepare_v2 = (fn_sqlite3_prepare_v2)GetProcAddress(
      sqlite_lib.hLib, "sqlite3_prepare_v2");
  sqlite_lib.step =
      (fn_sqlite3_step)GetProcAddress(sqlite_lib.hLib, "sqlite3_step");
  sqlite_lib.column_text = (fn_sqlite3_column_text)GetProcAddress(
      sqlite_lib.hLib, "sqlite3_column_text");
  sqlite_lib.column_count = (fn_sqlite3_column_count)GetProcAddress(
      sqlite_lib.hLib, "sqlite3_column_count");
  sqlite_lib.column_name = (fn_sqlite3_column_name)GetProcAddress(
      sqlite_lib.hLib, "sqlite3_column_name");
  sqlite_lib.finalize =
      (fn_sqlite3_finalize)GetProcAddress(sqlite_lib.hLib, "sqlite3_finalize");
  sqlite_lib.errmsg =
      (fn_sqlite3_errmsg)GetProcAddress(sqlite_lib.hLib, "sqlite3_errmsg");

  if (!sqlite_lib.open || !sqlite_lib.close || !sqlite_lib.prepare_v2 ||
      !sqlite_lib.step || !sqlite_lib.column_text || !sqlite_lib.column_count ||
      !sqlite_lib.column_name || !sqlite_lib.finalize || !sqlite_lib.errmsg) {
    FreeLibrary(sqlite_lib.hLib);
    memset(&sqlite_lib, 0, sizeof(SqliteLib));
    return FALSE;
  }

  return TRUE;
}

void sqlite_cleanup_lib(void) {
  if (sqlite_lib.hLib) {
    FreeLibrary(sqlite_lib.hLib);
    memset(&sqlite_lib, 0, sizeof(SqliteLib));
  }
}

static void sqlite_load_columns(sqlite3 *db, DbTreeCallback cb, void *user_data,
                                void *hTableItem, const char *table_name,
                                const char *conn_id) {
  void *hColParent = cb(user_data, hTableItem, NODE_OTHER, "Columns", conn_id,
                        "", "", table_name);

  char query[256];
  snprintf(query, sizeof(query), "PRAGMA table_info(%s);", table_name);

  sqlite3_stmt *stmt = NULL;
  if (sqlite_lib.prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite_lib.step(stmt) == SQLITE_ROW) {
      const char *col_name = (const char *)sqlite_lib.column_text(stmt, 1);
      const char *col_type = (const char *)sqlite_lib.column_text(stmt, 2);

      char display[256];
      snprintf(display, sizeof(display), "%s (%s)", col_name, col_type);
      cb(user_data, hColParent, NODE_COLUMN, display, conn_id, "", "",
         table_name);
    }
    sqlite_lib.finalize(stmt);
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
  if (sqlite_lib.prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite_lib.step(stmt) == SQLITE_ROW) {
      const char *ref_table = (const char *)sqlite_lib.column_text(stmt, 2);
      const char *from_col = (const char *)sqlite_lib.column_text(stmt, 3);
      const char *to_col = (const char *)sqlite_lib.column_text(stmt, 4);

      char display[512];
      snprintf(display, sizeof(display), "%s -> %s.%s", from_col, ref_table,
               to_col);
      cb(user_data, hFkParent, NODE_OTHER, display, conn_id, "", "",
         table_name);
    }
    sqlite_lib.finalize(stmt);
  }
}

void sqlite_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                      void *conn_handle) {
  if (!sqlite_init_lib()) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to load sqlite3.dll",
       info->id, "", "", "");
    return;
  }

  sqlite3 *db = NULL;
  if (sqlite_lib.open(info->database, &db) != SQLITE_OK) {
    cb(user_data, conn_handle, NODE_OTHER, "Failed to open SQLite file",
       info->id, "", "", "");
    if (db)
      sqlite_lib.close(db);
    return;
  }

  void *hDbItem = cb(user_data, conn_handle, NODE_DATABASE, "main", info->id,
                     "main", "", "");

  sqlite3_stmt *stmt = NULL;
  const char *query = "SELECT name FROM sqlite_master WHERE type='table' AND "
                      "name NOT LIKE 'sqlite_%';";
  if (sqlite_lib.prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite_lib.step(stmt) == SQLITE_ROW) {
      const char *table_name = (const char *)sqlite_lib.column_text(stmt, 0);
      void *hTableItem = cb(user_data, hDbItem, NODE_TABLE, table_name,
                            info->id, "main", "", table_name);
      sqlite_load_columns(db, cb, user_data, hTableItem, table_name, info->id);
      sqlite_load_foreign_keys(db, cb, user_data, hTableItem, table_name,
                               info->id);
    }
    sqlite_lib.finalize(stmt);
  }

  sqlite_lib.close(db);
}

void sqlite_run_query(ConnectionInfo *info, const char *active_db,
                      const char *query, DbHeaderCallback header_cb,
                      DbRowCallback row_cb, DbStatusCallback status_cb,
                      void *user_data) {
  (void)active_db;
  if (!sqlite_init_lib()) {
    status_cb(user_data, "Failed to load sqlite3.dll");
    return;
  }

  sqlite3 *db = NULL;
  if (sqlite_lib.open(info->database, &db) != SQLITE_OK) {
    status_cb(user_data,
              db ? sqlite_lib.errmsg(db) : "Failed to open SQLite database.");
    if (db)
      sqlite_lib.close(db);
    return;
  }

  sqlite3_stmt *stmt = NULL;
  if (sqlite_lib.prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    status_cb(user_data, sqlite_lib.errmsg(db));
    sqlite_lib.close(db);
    return;
  }

  int cols = sqlite_lib.column_count(stmt);
  for (int c = 0; c < cols; c++) {
    header_cb(user_data, c, sqlite_lib.column_name(stmt, c));
  }

  int r = 0;
  int step_res;
  while ((step_res = sqlite_lib.step(stmt)) == SQLITE_ROW) {
    for (int c = 0; c < cols; c++) {
      const char *val = (const char *)sqlite_lib.column_text(stmt, c);
      row_cb(user_data, r, c, val ? val : "NULL");
    }
    r++;
  }

  char status_msg[128];
  snprintf(status_msg, sizeof(status_msg), "OK: %d row(s)", r);
  status_cb(user_data, status_msg);

  sqlite_lib.finalize(stmt);
  sqlite_lib.close(db);
}
