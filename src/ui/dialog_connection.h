#ifndef UI_DIALOG_CONNECTION_H
#define UI_DIALOG_CONNECTION_H
#include <gtk/gtk.h>
typedef enum {
    POSTGRESQL,
    MYSQL,
    SQLITE
} DatabaseType;

void show_create_connection_dialog(GtkWidget *parent);
#endif