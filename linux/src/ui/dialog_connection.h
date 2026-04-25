#ifndef UI_DIALOG_CONNECTION_H
#define UI_DIALOG_CONNECTION_H
#include <gtk/gtk.h>
#include "sidebar.h"
#include "connection_store.h"

typedef enum {
    POSTGRESQL,
    DB_MYSQL,
    SQLITE
} DatabaseType;

typedef struct {
    GtkWidget *name;        // Connection display name
    GtkWidget *host;
    GtkWidget *port;
    GtkWidget *password;
    GtkWidget *username;
    GtkWidget *db;

    GtkWidget *err_name;
    GtkWidget *err_host;
    GtkWidget *err_port;
    GtkWidget *err_username;
    GtkWidget *err_password;
} SqlForm;

typedef struct {
    Sidebar *sidebar;
    DatabaseType db_type;
} DbSelectContext;

typedef struct {
    SqlForm *form;
    Sidebar *sidebar;
    ConnectionStore *conn_store;
    DatabaseType db_type;
} SqlConnectContext;

void show_create_connection_dialog(GtkWidget *parent, gpointer data);
#endif // UI_DIALOG_CONNECTION_H