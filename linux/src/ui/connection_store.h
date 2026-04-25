#ifndef UI_CONNECTION_STORE_H
#define UI_CONNECTION_STORE_H

#include <gtk/gtk.h>

typedef enum {
    DB_TYPE_POSTGRESQL,
    DB_TYPE_MYSQL,
    DB_TYPE_SQLITE
} DbType;

typedef struct {
    char *id;           
    char *name;         
    DbType type;
    char *host;
    char *port;
    char *username;
    char *password;
    char *database;
} ConnectionInfo;

typedef struct _ConnectionStore {
    GList *connections;  
    char *config_path;
} ConnectionStore;

ConnectionStore *connection_store_new(void);

void connection_store_free(ConnectionStore *store);
void connection_store_add(ConnectionStore *store, ConnectionInfo *info);
void connection_store_remove(ConnectionStore *store, const char *id);
ConnectionInfo *connection_store_get(ConnectionStore *store, const char *id);
gboolean connection_store_save(ConnectionStore *store);
gboolean connection_store_load(ConnectionStore *store);
ConnectionInfo *connection_info_new(const char *name, DbType type,
                                    const char *host, const char *port,
                                    const char *username, const char *password,
                                    const char *database);
void connection_info_free(ConnectionInfo *info);
char *connection_store_generate_id(void);

#endif
