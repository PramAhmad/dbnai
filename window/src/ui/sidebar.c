#include "sidebar.h"
#include "../db/mysql.h"
#include "../db/postgres.h"
#include "../db/sqlite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LRESULT CALLBACK SidebarWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam) {
  if (msg == WM_COMMAND || msg == WM_NOTIFY) {
    return SendMessageA(GetParent(hwnd), msg, wParam, lParam);
  }
  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

Sidebar *create_sidebar(HWND hwndParent, HINSTANCE hInst,
                        ConnectionStore *store) {
  Sidebar *sidebar = malloc(sizeof(Sidebar));
  if (!sidebar)
    return NULL;

  sidebar->conn_store = store;

  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.lpfnWndProc = SidebarWndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = "DBMRAP_SIDEBAR";
  RegisterClassExA(&wc);

  sidebar->hwndContainer = CreateWindowExA(
      0, "DBMRAP_SIDEBAR", "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0,
      0, hwndParent, NULL, hInst, NULL);

  sidebar->hwndButton = CreateWindowExA(
      0, "BUTTON", "Create Connection", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      5, 5, 190, 30, sidebar->hwndContainer, (HMENU)IDC_BTN_CREATE_CONN, hInst,
      NULL);

  sidebar->hwndTreeView =
      CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, "",
                      WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS |
                          TVS_LINESATROOT | TVS_SHOWSELALWAYS,
                      5, 40, 190, 100, sidebar->hwndContainer,
                      (HMENU)IDC_TREEVIEW, hInst, NULL);

  return sidebar;
}

void resize_sidebar(Sidebar *sidebar, int x, int y, int width, int height) {
  if (!sidebar)
    return;
  MoveWindow(sidebar->hwndContainer, x, y, width, height, TRUE);
  MoveWindow(sidebar->hwndButton, 5, 5, width - 10, 30, TRUE);
  MoveWindow(sidebar->hwndTreeView, 5, 40, width - 10, height - 45, TRUE);
}

static HTREEITEM insert_tree_item(HWND hwndTV, HTREEITEM hParent,
                                  const char *text, TreeNodeData *data) {
  TVINSERTSTRUCTA tvis = {0};
  tvis.hParent = hParent;
  tvis.hInsertAfter = TVI_LAST;
  tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
  tvis.item.pszText = (LPSTR)text;
  tvis.item.lParam = (LPARAM)data;
  return (HTREEITEM)SendMessageA(hwndTV, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}

static void *win32_tree_callback(void *user_data, void *parent_handle,
                                 NodeKind kind, const char *label,
                                 const char *conn_id, const char *db_name,
                                 const char *schema_name,
                                 const char *table_name) {
  Sidebar *sidebar = (Sidebar *)user_data;
  HTREEITEM hParent = (HTREEITEM)parent_handle;

  TreeNodeData *data = malloc(sizeof(TreeNodeData));
  if (data) {
    data->kind = kind;
    data->conn_id = conn_id ? strdup(conn_id) : NULL;
    data->db_name = db_name ? strdup(db_name) : NULL;
    data->schema_name = schema_name ? strdup(schema_name) : NULL;
    data->table_name = table_name ? strdup(table_name) : NULL;
  }

  HTREEITEM hItem =
      insert_tree_item(sidebar->hwndTreeView, hParent, label, data);
  return (void *)hItem;
}

void add_connection_node(Sidebar *sidebar, ConnectionInfo *info) {
  if (!sidebar || !info)
    return;

  TreeNodeData *data = malloc(sizeof(TreeNodeData));
  if (data) {
    data->kind = NODE_CONNECTION;
    data->conn_id = info->id ? strdup(info->id) : NULL;
    data->db_name = info->database ? strdup(info->database) : NULL;
    data->schema_name = NULL;
    data->table_name = NULL;
  }

  HTREEITEM hConn =
      insert_tree_item(sidebar->hwndTreeView, TVI_ROOT, info->name, data);

  if (info->type == DB_TYPE_POSTGRESQL) {
    pg_load_tree(info, win32_tree_callback, sidebar, (void *)hConn);
  } else if (info->type == DB_TYPE_MYSQL) {
    mysql_load_tree(info, win32_tree_callback, sidebar, (void *)hConn);
  } else if (info->type == DB_TYPE_SQLITE) {
    sqlite_load_tree(info, win32_tree_callback, sidebar, (void *)hConn);
  }
}

void load_saved_connections_to_tree(Sidebar *sidebar) {
  if (!sidebar || !sidebar->conn_store)
    return;

  // Clear all items
  SendMessageA(sidebar->hwndTreeView, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);

  for (int i = 0; i < sidebar->conn_store->count; i++) {
    ConnectionInfo *info = sidebar->conn_store->connections[i];
    add_connection_node(sidebar, info);
  }
}

void free_tree_node_data(TreeNodeData *data) {
  if (data) {
    free(data->conn_id);
    free(data->db_name);
    free(data->schema_name);
    free(data->table_name);
    free(data);
  }
}

void sidebar_refresh_connection_by_id(Sidebar *sidebar, const char *conn_id) {
  if (!sidebar || !conn_id)
    return;

  HWND hwndTV = sidebar->hwndTreeView;
  HTREEITEM hItem = (HTREEITEM)SendMessageA(hwndTV, TVM_GETNEXTITEM, TVGN_ROOT, 0);
  while (hItem) {
    TVITEMA tvi = {0};
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hItem;
    SendMessageA(hwndTV, TVM_GETITEMA, 0, (LPARAM)&tvi);
    TreeNodeData *data = (TreeNodeData *)tvi.lParam;
    if (data && data->kind == NODE_CONNECTION && data->conn_id && strcmp(data->conn_id, conn_id) == 0) {
      ConnectionInfo *info = connection_store_get(sidebar->conn_store, conn_id);
      if (info) {
        SendMessageA(hwndTV, TVM_DELETEITEM, 0, (LPARAM)hItem);
        add_connection_node(sidebar, info);
      }
      break;
    }
    hItem = (HTREEITEM)SendMessageA(hwndTV, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
  }
}
