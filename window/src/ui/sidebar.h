#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include "../../../shared/db_types.h"
#include "store.h"
#include <commctrl.h>
#include <windows.h>

#define IDC_BTN_CREATE_CONN 4001
#define IDC_TREEVIEW 4002

typedef struct {
  NodeKind kind;
  char *conn_id;
  char *db_name;
} TreeNodeData;

typedef struct {
  HWND hwndContainer;
  HWND hwndButton;
  HWND hwndTreeView;
  ConnectionStore *conn_store;
} Sidebar;

Sidebar *create_sidebar(HWND hwndParent, HINSTANCE hInst,
                        ConnectionStore *store);
void resize_sidebar(Sidebar *sidebar, int x, int y, int width, int height);
void load_saved_connections_to_tree(Sidebar *sidebar);
void add_connection_node(Sidebar *sidebar, ConnectionInfo *info);
void free_tree_node_data(TreeNodeData *data);

#endif