#ifndef SHARED_DB_TYPES_H
#define SHARED_DB_TYPES_H

typedef enum {
  NODE_CONNECTION,
  NODE_DATABASE,
  NODE_SCHEMA,
  NODE_TABLE,
  NODE_COLUMN,
  NODE_OTHER
} NodeKind;

typedef enum { DB_TYPE_POSTGRESQL, DB_TYPE_MYSQL, DB_TYPE_SQLITE } DbType;

typedef void *(*DbTreeCallback)(void *user_data, void *parent_handle,
                                NodeKind kind, const char *label,
                                const char *conn_id, const char *db_name,
                                const char *schema_name,
                                const char *table_name);

typedef void (*DbHeaderCallback)(void *user_data, int col_idx,
                                 const char *col_name);

typedef void (*DbRowCallback)(void *user_data, int row_idx, int col_idx,
                              const char *value);

typedef void (*DbStatusCallback)(void *user_data, const char *status_msg);

#endif
