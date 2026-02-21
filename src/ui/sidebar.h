#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include <gtk/gtk.h>
typedef enum {
    POSTGRESQL,
    MYSQL,
    SQLITE
} DatabaseType;

GtkWidget* create_sidebar(void);

#endif
