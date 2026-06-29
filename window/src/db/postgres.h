#ifndef DB_POSTGRES_H
#define DB_POSTGRES_H

#include "../../../shared/db_types.h"
#include "db_types.h"
#include <windows.h>

typedef void *PGconn;
typedef void *PGresult;

typedef enum {
  PGRES_EMPTY_QUERY = 0,
  PGRES_COMMAND_OK,
  PGRES_TUPLES_OK,
  PGRES_COPY_OUT,
  PGRES_COPY_IN,
  PGRES_BAD_RESPONSE,
  PGRES_NONFATAL_ERROR,
  PGRES_FATAL_ERROR,
  PGRES_COPY_BOTH,
  PGRES_SINGLE_TUPLE
} ExecStatusType;

typedef enum { CONNECTION_OK, CONNECTION_BAD } ConnStatusType;

typedef PGconn *(*fn_PQconnectdb)(const char *conninfo);
typedef ConnStatusType (*fn_PQstatus)(const PGconn *conn);
typedef char *(*fn_PQerrorMessage)(const PGconn *conn);
typedef void (*fn_PQfinish)(PGconn *conn);
typedef PGresult *(*fn_PQexec)(PGconn *conn, const char *query);
typedef ExecStatusType (*fn_PQresultStatus)(const PGresult *res);
typedef int (*fn_PQntuples)(const PGresult *res);
typedef int (*fn_PQnfields)(const PGresult *res);
typedef char *(*fn_PQfname)(const PGresult *res, int field_num);
typedef char *(*fn_PQgetvalue)(const PGresult *res, int tup_num, int field_num);
typedef int (*fn_PQgetisnull)(const PGresult *res, int tup_num, int field_num);
typedef char *(*fn_PQcmdTuples)(PGresult *res);
typedef char *(*fn_PQcmdStatus)(PGresult *res);
typedef void (*fn_PQclear)(PGresult *res);

typedef struct {
  HMODULE hLib;
  fn_PQconnectdb connectdb;
  fn_PQstatus status;
  fn_PQerrorMessage errorMessage;
  fn_PQfinish finish;
  fn_PQexec exec;
  fn_PQresultStatus resultStatus;
  fn_PQntuples ntuples;
  fn_PQnfields nfields;
  fn_PQfname fname;
  fn_PQgetvalue getvalue;
  fn_PQgetisnull getisnull;
  fn_PQcmdTuples cmdTuples;
  fn_PQcmdStatus cmdStatus;
  fn_PQclear clear;
} PostgresLib;

extern PostgresLib pg_lib;

BOOL postgres_init(void);
void postgres_cleanup(void);

void pg_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                  void *conn_handle);
void pg_run_query(ConnectionInfo *info, const char *active_db,
                  const char *query, DbHeaderCallback header_cb,
                  DbRowCallback row_cb, DbStatusCallback status_cb,
                  void *user_data);

#endif
