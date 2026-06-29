#ifndef DB_SQLITE_H
#define DB_SQLITE_H

#include "../../../shared/db_types.h"
#include "db_types.h"
#include <windows.h>

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

#define SQLITE_OK 0     /* Success result */
#define SQLITE_ROW 100  /* sqlite3_step() hass another row ready */
#define SQLITE_DONE 101 /* sqlite3_step() has finished executing */

typedef int (*fn_sqlite3_open)(const char *filename, sqlite3 **ppDb);
typedef int (*fn_sqlite3_close)(sqlite3 *);
typedef int (*fn_sqlite3_prepare_v2)(sqlite3 *db, const char *zSql, int nByte,
                                     sqlite3_stmt **ppStmt,
                                     const char **pzTail);
typedef int (*fn_sqlite3_step)(sqlite3_stmt *);
typedef const unsigned char *(*fn_sqlite3_column_text)(sqlite3_stmt *,
                                                       int iCol);
typedef int (*fn_sqlite3_column_count)(sqlite3_stmt *pStmt);
typedef const char *(*fn_sqlite3_column_name)(sqlite3_stmt *, int N);
typedef int (*fn_sqlite3_finalize)(sqlite3_stmt *pStmt);
typedef const char *(*fn_sqlite3_errmsg)(sqlite3 *);

typedef struct {
  HMODULE hLib;
  fn_sqlite3_open open;
  fn_sqlite3_close close;
  fn_sqlite3_prepare_v2 prepare_v2;
  fn_sqlite3_step step;
  fn_sqlite3_column_text column_text;
  fn_sqlite3_column_count column_count;
  fn_sqlite3_column_name column_name;
  fn_sqlite3_finalize finalize;
  fn_sqlite3_errmsg errmsg;
} SqliteLib;

extern SqliteLib sqlite_lib;

BOOL sqlite_init_lib(void);
void sqlite_cleanup_lib(void);

void sqlite_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                      void *conn_handle);
void sqlite_run_query(ConnectionInfo *info, const char *active_db,
                      const char *query, DbHeaderCallback header_cb,
                      DbRowCallback row_cb, DbStatusCallback status_cb,
                      void *user_data);

#endif
