#ifndef DB_MYSQL_H
#define DB_MYSQL_H

#include "../../../shared/db_types.h"
#include "db_types.h"
#include <windows.h>

typedef void MYSQL;
typedef void MYSQL_RES;
typedef char **MYSQL_ROW;

typedef struct {
  char *name;
} MYSQL_FIELD;

typedef MYSQL *(*fn_mysql_init)(MYSQL *mysql);
typedef MYSQL *(*fn_mysql_real_connect)(MYSQL *mysql, const char *host,
                                        const char *user, const char *passwd,
                                        const char *db, unsigned int port,
                                        const char *unix_socket,
                                        unsigned long clientflag);
typedef const char *(*fn_mysql_error)(MYSQL *mysql);
typedef void (*fn_mysql_close)(MYSQL *mysql);
typedef int (*fn_mysql_query)(MYSQL *mysql, const char *q);
typedef MYSQL_RES *(*fn_mysql_store_result)(MYSQL *mysql);
typedef MYSQL_ROW (*fn_mysql_fetch_row)(MYSQL_RES *result);
typedef unsigned int (*fn_mysql_num_fields)(MYSQL_RES *res);
typedef void (*fn_mysql_free_result)(MYSQL_RES *result);
typedef unsigned long long (*fn_mysql_affected_rows)(MYSQL *mysql);
typedef unsigned long (*fn_mysql_num_rows)(MYSQL_RES *res);
typedef MYSQL_FIELD *(*fn_mysql_fetch_field_direct)(MYSQL_RES *res,
                                                    unsigned int fieldnr);

typedef struct {
  HMODULE hLib;
  fn_mysql_init init;
  fn_mysql_real_connect real_connect;
  fn_mysql_error error;
  fn_mysql_close close;
  fn_mysql_query query;
  fn_mysql_store_result store_result;
  fn_mysql_fetch_row fetch_row;
  fn_mysql_num_fields num_fields;
  fn_mysql_free_result free_result;
  fn_mysql_affected_rows affected_rows;
  fn_mysql_num_rows num_rows;
  fn_mysql_fetch_field_direct fetch_field_direct;
} MySqlLib;

extern MySqlLib mysql_lib;

BOOL mysql_init_lib(void);
void mysql_cleanup_lib(void);

void mysql_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                     void *conn_handle);
void mysql_run_query(ConnectionInfo *info, const char *active_db,
                     const char *query, DbHeaderCallback header_cb,
                     DbRowCallback row_cb, DbStatusCallback status_cb,
                     void *user_data);

#endif
