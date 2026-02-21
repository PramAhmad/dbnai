#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include <gtk/gtk.h>
typedef enum {
    DB_POSTGRESQL,
    DB_MYSQL,
    DB_SQLITE
} DatabaseType;

GtkWidget* create_sidebar(void);

#endif
