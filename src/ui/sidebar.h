#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include <gtk/gtk.h>
typedef struct {
    GtkWidget *container;
    GtkTreeStore *store;
    GtkWidget *treeview;
} Sidebar;

GtkWidget* create_sidebar(void);

#endif
