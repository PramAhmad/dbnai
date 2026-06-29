#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include "../../../shared/db_types.h"
#include "connection_store.h"
#include <gtk/gtk.h>

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

GtkWidget *create_sidebar(void);
Sidebar *sidebar_from_widget(GtkWidget *container);
void sidebar_add_connection(Sidebar *sidebar, ConnectionInfo *info);

#endif
