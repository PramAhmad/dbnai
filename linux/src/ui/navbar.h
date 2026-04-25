#ifndef UI_NAVBAR_H
#define UI_NAVBAR_H

#include <gtk/gtk.h>

typedef struct {
    char *label;
    char *action;
} MenuItem;

GtkWidget* create_navbar(GtkApplication *app);

#endif
