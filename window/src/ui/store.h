#ifndef UI_STORE_H
#define UI_STORE_H

#include "../db/db_types.h"
#include <windows.h>

typedef struct {
  ConnectionInfo **connections;
  int count;
  char config_path[MAX_PATH];
} ConnectionStore;

ConnectionStore *connection_store_new(void);
void connection_store_free(ConnectionStore *store);
void connection_store_add(ConnectionStore *store, ConnectionInfo *info);
void connection_store_remove(ConnectionStore *store, const char *id);
ConnectionInfo *connection_store_get(ConnectionStore *store, const char *id);
BOOL connection_store_save(ConnectionStore *store);
BOOL connection_store_load(ConnectionStore *store);
ConnectionInfo *connection_info_new(const char *name, DbType type,
                                    const char *host, const char *port,
                                    const char *username, const char *password,
                                    const char *database);
void connection_info_free(ConnectionInfo *info);
char *connection_store_generate_id(void);

#endif
