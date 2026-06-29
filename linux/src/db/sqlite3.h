#ifndef MINIMAL_SQLITE3_H
#define MINIMAL_SQLITE3_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

#define SQLITE_OK 0
#define SQLITE_ROW 100

int sqlite3_open(const char *filename, sqlite3 **ppDb);
int sqlite3_close(sqlite3 *db);
int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte,
                       sqlite3_stmt **ppStmt, const char **pzTail);
int sqlite3_step(sqlite3_stmt *stmt);
int sqlite3_finalize(sqlite3_stmt *pStmt);
int sqlite3_column_count(sqlite3_stmt *pStmt);
const char *sqlite3_column_name(sqlite3_stmt *stmt, int N);
const unsigned char *sqlite3_column_text(sqlite3_stmt *stmt, int iCol);
const char *sqlite3_errmsg(sqlite3 *db);

#ifdef __cplusplus
}
#endif

#endif
