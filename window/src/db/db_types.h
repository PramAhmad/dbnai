#ifndef DB_DB_TYPES_H
#define DB_DB_TYPES_H

#include "../../../shared/db_types.h"

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

#endif
