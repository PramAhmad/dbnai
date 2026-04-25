#ifndef POSTGRES_H
#define POSTGRES_H
#include <libpq-fe.h>
#include "dialog_connection.h"

void validate_pg_form(SqlForm *form);
void connect_postgresql(SqlConnectContext *ctx);

#endif