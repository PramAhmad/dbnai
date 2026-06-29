#include "store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *connection_store_generate_id(void) {
  static int counter = 0;
  char *id = malloc(64);
  if (id) {
    snprintf(id, 64, "conn_%d", counter++);
  }
  return id;
}

ConnectionInfo *connection_info_new(const char *name, DbType type,
                                    const char *host, const char *port,
                                    const char *username, const char *password,
                                    const char *database) {
  ConnectionInfo *info = malloc(sizeof(ConnectionInfo));
  if (!info)
    return NULL;
  info->id = connection_store_generate_id();
  info->name = name ? strdup(name) : strdup("");
  info->type = type;
  info->host = host ? strdup(host) : strdup("");
  info->port = port ? strdup(port) : strdup("");
  info->username = username ? strdup(username) : strdup("");
  info->password = password ? strdup(password) : strdup("");
  info->database = database ? strdup(database) : strdup("");
  return info;
}

void connection_info_free(ConnectionInfo *info) {
  if (!info)
    return;
  free(info->id);
  free(info->name);
  free(info->host);
  free(info->port);
  free(info->username);
  free(info->password);
  free(info->database);
  free(info);
}

ConnectionStore *connection_store_new(void) {
  ConnectionStore *store = malloc(sizeof(ConnectionStore));
  if (!store)
    return NULL;
  store->connections = NULL;
  store->count = 0;
  memset(store->config_path, 0, sizeof(store->config_path));

  char *appdata = getenv("APPDATA");
  if (appdata) {
    snprintf(store->config_path, sizeof(store->config_path), "%s\\dbmrap",
             appdata);
    CreateDirectoryA(store->config_path, NULL);
    strncat(store->config_path, "\\connections.ini",
            sizeof(store->config_path) - strlen(store->config_path) - 1);
  }
  return store;
}

void connection_store_free(ConnectionStore *store) {
  if (!store)
    return;
  for (int i = 0; i < store->count; i++) {
    connection_info_free(store->connections[i]);
  }
  free(store->connections);
  free(store);
}

void connection_store_add(ConnectionStore *store, ConnectionInfo *info) {
  if (!store || !info)
    return;
  store->connections = realloc(store->connections,
                               sizeof(ConnectionInfo *) * (store->count + 1));
  if (store->connections) {
    store->connections[store->count] = info;
    store->count++;
  }
}

void connection_store_remove(ConnectionStore *store, const char *id) {
  if (!store || !id)
    return;
  int index = -1;
  for (int i = 0; i < store->count; i++) {
    if (strcmp(store->connections[i]->id, id) == 0) {
      index = i;
      break;
    }
  }
  if (index != -1) {
    connection_info_free(store->connections[index]);
    for (int i = index; i < store->count - 1; i++) {
      store->connections[i] = store->connections[i + 1];
    }
    store->count--;
    if (store->count > 0) {
      store->connections =
          realloc(store->connections, sizeof(ConnectionInfo *) * store->count);
    } else {
      free(store->connections);
      store->connections = NULL;
    }
  }
}

ConnectionInfo *connection_store_get(ConnectionStore *store, const char *id) {
  if (!store || !id)
    return NULL;
  for (int i = 0; i < store->count; i++) {
    if (strcmp(store->connections[i]->id, id) == 0) {
      return store->connections[i];
    }
  }
  return NULL;
}

BOOL connection_store_save(ConnectionStore *store) {
  if (!store || strlen(store->config_path) == 0)
    return FALSE;

  FILE *f = fopen(store->config_path, "w");
  if (f)
    fclose(f);

  char count_str[32];
  snprintf(count_str, sizeof(count_str), "%d", store->count);
  WritePrivateProfileStringA("Settings", "Count", count_str,
                             store->config_path);

  for (int i = 0; i < store->count; i++) {
    ConnectionInfo *info = store->connections[i];
    char group[64];
    snprintf(group, sizeof(group), "Connection_%d", i);

    WritePrivateProfileStringA(group, "id", info->id, store->config_path);
    WritePrivateProfileStringA(group, "name", info->name, store->config_path);

    const char *type_str = "postgresql";
    if (info->type == DB_TYPE_MYSQL)
      type_str = "mysql";
    else if (info->type == DB_TYPE_SQLITE)
      type_str = "sqlite";
    WritePrivateProfileStringA(group, "type", type_str, store->config_path);

    WritePrivateProfileStringA(group, "host", info->host, store->config_path);
    WritePrivateProfileStringA(group, "port", info->port, store->config_path);
    WritePrivateProfileStringA(group, "username", info->username,
                               store->config_path);
    WritePrivateProfileStringA(group, "password", info->password,
                               store->config_path);
    WritePrivateProfileStringA(group, "database", info->database,
                               store->config_path);
  }
  return TRUE;
}

BOOL connection_store_load(ConnectionStore *store) {
  if (!store || strlen(store->config_path) == 0)
    return FALSE;

  char count_str[32];
  GetPrivateProfileStringA("Settings", "Count", "0", count_str,
                           sizeof(count_str), store->config_path);
  int count = atoi(count_str);

  for (int i = 0; i < count; i++) {
    char group[64];
    snprintf(group, sizeof(group), "Connection_%d", i);

    char id[128], name[128], type_str[64], host[128], port[64], username[128],
        password[128], database[MAX_PATH];

    GetPrivateProfileStringA(group, "id", "", id, sizeof(id),
                             store->config_path);
    if (strlen(id) == 0)
      continue;

    GetPrivateProfileStringA(group, "name", "", name, sizeof(name),
                             store->config_path);
    GetPrivateProfileStringA(group, "type", "postgresql", type_str,
                             sizeof(type_str), store->config_path);

    DbType type = DB_TYPE_POSTGRESQL;
    if (strcmp(type_str, "mysql") == 0)
      type = DB_TYPE_MYSQL;
    else if (strcmp(type_str, "sqlite") == 0)
      type = DB_TYPE_SQLITE;

    GetPrivateProfileStringA(group, "host", "", host, sizeof(host),
                             store->config_path);
    GetPrivateProfileStringA(group, "port", "", port, sizeof(port),
                             store->config_path);
    GetPrivateProfileStringA(group, "username", "", username, sizeof(username),
                             store->config_path);
    GetPrivateProfileStringA(group, "password", "", password, sizeof(password),
                             store->config_path);
    GetPrivateProfileStringA(group, "database", "", database, sizeof(database),
                             store->config_path);

    ConnectionInfo *info = connection_info_new(name, type, host, port, username,
                                               password, database);
    if (info) {
      free(info->id);
      info->id = strdup(id);
      connection_store_add(store, info);
    }
  }
  return TRUE;
}
