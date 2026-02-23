#ifndef UI_DIALOG_CONNECTION_H
#define UI_DIALOG_CONNECTION_H
#include <gtk/gtk.h>
#include "sidebar.h"
typedef enum {
    POSTGRESQL,
    MYSQL,
    SQLITE
} DatabaseType;

typedef struct {
    GtkWidget *host;
    GtkWidget *port;
    GtkWidget *password;
    GtkWidget *username;
    GtkWidget *db;

    GtkWidget *err_host;
    GtkWidget *err_port;
    GtkWidget *err_username;
    GtkWidget *err_password;
} PgForm;

typedef struct {
    Sidebar *sidebar;
    DatabaseType db_type;
} DbSelectContext;

typedef struct {
    PgForm *form;
    Sidebar *sidebar;
} PgConnectContext;

void show_create_connection_dialog(GtkWidget *parent, gpointer data);
#endif