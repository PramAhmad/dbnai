#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include <gtk/gtk.h>

typedef struct _ConnectionStore ConnectionStore;

typedef struct {
    GtkWidget *container;
    GtkTreeStore *store;
    GtkWidget *treeview;
    ConnectionStore *conn_store;
} Sidebar;

typedef enum {
    SIDEBAR_COL_LABEL = 0,
    SIDEBAR_COL_NODE_KIND,
    SIDEBAR_COL_CONN_ID,
    SIDEBAR_COL_DB_NAME,
    SIDEBAR_N_COLS
} SidebarColumn;

typedef enum {
    SIDEBAR_NODE_UNKNOWN = 0,
    SIDEBAR_NODE_CONNECTION,
    SIDEBAR_NODE_DATABASE,
    SIDEBAR_NODE_OTHER
} SidebarNodeKind;

GtkWidget* create_sidebar(void);
Sidebar* sidebar_from_widget(GtkWidget *container);

#endif
