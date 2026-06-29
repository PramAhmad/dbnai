#ifndef DB_POSTGRES_H
#define DB_POSTGRES_H

#include "../../../shared/db_types.h"
#include "../ui/connection_store.h"

void pg_load_tree(ConnectionInfo *info, DbTreeCallback cb, void *user_data,
                  void *conn_handle);
void pg_run_query(ConnectionInfo *info, const char *active_db,
                  const char *query, DbHeaderCallback header_cb,
                  DbRowCallback row_cb, DbStatusCallback status_cb,
                  void *user_data);

#endif