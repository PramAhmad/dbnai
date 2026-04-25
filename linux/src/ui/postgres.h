#ifndef POSTGRES_H
#define POSTGRES_H
#include <libpq-fe.h>
#include "dialog_connection.h"
#include "connection_store.h"
#include "sidebar.h"

typedef struct {
    GtkWidget *entry;
    GtkWidget *err_label;
    const char *err_msg;
    const char *field_name;
} FormField;

void validate_pg_form(SqlForm *form);
void connect_postgresql(SqlConnectContext *ctx);
void load_saved_connection(Sidebar *sidebar, ConnectionInfo *info);

#endif