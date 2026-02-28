#ifndef POSTGRES_H
#define POSTGRES_H
#include <libpq-fe.h>
#include "dialog_connection.h"

typedef struct {
    GtkWidget *entry;
    GtkWidget *err_label;
    const char *err_msg;
    const char *field_name;
} FormField;

void validate_pg_form(PgForm *form);
void connect_postgresql(PgConnectContext *ctx);

#endif