#include "connection_store.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *DB_TYPE_STRINGS[] = {"postgresql", "mysql", "sqlite"};

static char *get_config_path(void) {
    const char *config_dir = g_get_user_config_dir();
    char *app_dir = g_build_filename(config_dir, "dbmrap", NULL);
    
    g_mkdir_with_parents(app_dir, 0755);
    
    char *path = g_build_filename(app_dir, "connections.ini", NULL);
    g_free(app_dir);
    return path;
}

char *connection_store_generate_id(void) {
    static int counter = 0;
    return g_strdup_printf("conn_%ld_%d", time(NULL), counter++);
}

ConnectionInfo *connection_info_new(const char *name, DbType type,
                                    const char *host, const char *port,
                                    const char *username, const char *password,
                                    const char *database) {
    ConnectionInfo *info = g_malloc(sizeof(ConnectionInfo));
    info->id = connection_store_generate_id();
    info->name = g_strdup(name);
    info->type = type;
    info->host = g_strdup(host);
    info->port = g_strdup(port);
    info->username = g_strdup(username);
    info->password = g_strdup(password);
    info->database = g_strdup(database);
    return info;
}

void connection_info_free(ConnectionInfo *info) {
    if (!info) return;
    g_free(info->id);
    g_free(info->name);
    g_free(info->host);
    g_free(info->port);
    g_free(info->username);
    g_free(info->password);
    g_free(info->database);
    g_free(info);
}

ConnectionStore *connection_store_new(void) {
    ConnectionStore *store = g_malloc(sizeof(ConnectionStore));
    store->connections = NULL;
    store->config_path = get_config_path();
    return store;
}

void connection_store_free(ConnectionStore *store) {
    if (!store) return;
    g_list_free_full(store->connections, (GDestroyNotify)connection_info_free);
    g_free(store->config_path);
    g_free(store);
}

void connection_store_add(ConnectionStore *store, ConnectionInfo *info) {
    store->connections = g_list_append(store->connections, info);
}

void connection_store_remove(ConnectionStore *store, const char *id) {
    for (GList *l = store->connections; l != NULL; l = l->next) {
        ConnectionInfo *info = (ConnectionInfo *)l->data;
        if (strcmp(info->id, id) == 0) {
            store->connections = g_list_remove(store->connections, info);
            connection_info_free(info);
            break;
        }
    }
}

ConnectionInfo *connection_store_get(ConnectionStore *store, const char *id) {
    for (GList *l = store->connections; l != NULL; l = l->next) {
        ConnectionInfo *info = (ConnectionInfo *)l->data;
        if (strcmp(info->id, id) == 0) {
            return info;
        }
    }
    return NULL;
}

// Save connections ke config file
gboolean connection_store_save(ConnectionStore *store) {
    GKeyFile *keyfile = g_key_file_new();
    
    int index = 0;
    for (GList *l = store->connections; l != NULL; l = l->next) {
        ConnectionInfo *info = (ConnectionInfo *)l->data;
        char group[64];
        snprintf(group, sizeof(group), "Connection_%d", index++);
        
        g_key_file_set_string(keyfile, group, "id", info->id);
        g_key_file_set_string(keyfile, group, "name", info->name);
        g_key_file_set_string(keyfile, group, "type", DB_TYPE_STRINGS[info->type]);
        g_key_file_set_string(keyfile, group, "host", info->host);
        g_key_file_set_string(keyfile, group, "port", info->port);
        g_key_file_set_string(keyfile, group, "username", info->username);
        g_key_file_set_string(keyfile, group, "password", info->password);
        g_key_file_set_string(keyfile, group, "database", info->database ? info->database : "");
    }
    
    GError *error = NULL;
    gboolean result = g_key_file_save_to_file(keyfile, store->config_path, &error);
    if (error) {
        g_print("Failed to save connections: %s\n", error->message);
        g_error_free(error);
    }
    
    g_key_file_free(keyfile);
    return result;
}

static DbType string_to_db_type(const char *str) {
    if (strcmp(str, "postgresql") == 0) return DB_TYPE_POSTGRESQL;
    if (strcmp(str, "mysql") == 0) return DB_TYPE_MYSQL;
    if (strcmp(str, "sqlite") == 0) return DB_TYPE_SQLITE;
    return DB_TYPE_POSTGRESQL;
}


// Load connections dari config file
gboolean connection_store_load(ConnectionStore *store) {
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    
    if (!g_key_file_load_from_file(keyfile, store->config_path, G_KEY_FILE_NONE, &error)) {
        if (error->code != G_FILE_ERROR_NOENT) {
            g_print("Failed to load connections: %s\n", error->message);
        }
        g_error_free(error);
        g_key_file_free(keyfile);
        return FALSE;
    }
    
    gsize num_groups;
    gchar **groups = g_key_file_get_groups(keyfile, &num_groups);
    
    for (gsize i = 0; i < num_groups; i++) {
        ConnectionInfo *info = g_malloc(sizeof(ConnectionInfo));
        
        info->id = g_key_file_get_string(keyfile, groups[i], "id", NULL);
        info->name = g_key_file_get_string(keyfile, groups[i], "name", NULL);
        char *type_str = g_key_file_get_string(keyfile, groups[i], "type", NULL);
        info->type = string_to_db_type(type_str);
        g_free(type_str);
        info->host = g_key_file_get_string(keyfile, groups[i], "host", NULL);
        info->port = g_key_file_get_string(keyfile, groups[i], "port", NULL);
        info->username = g_key_file_get_string(keyfile, groups[i], "username", NULL);
        info->password = g_key_file_get_string(keyfile, groups[i], "password", NULL);
        info->database = g_key_file_get_string(keyfile, groups[i], "database", NULL);
        
        store->connections = g_list_append(store->connections, info);
    }
    
    g_strfreev(groups);
    g_key_file_free(keyfile);
    return TRUE;
}
