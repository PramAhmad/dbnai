#include "db/mysql.h"
#include "db/postgres.h"
#include "db/sqlite.h"
#include "ui/content.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/sidebar.h"
#include "ui/store.h"
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define IDM_SIDEBAR_DISCONNECT   6001
#define IDM_SIDEBAR_CREATE_DB    6002
#define IDM_SIDEBAR_DROP_DB      6003
#define IDM_SIDEBAR_SELECT_100   6004
#define IDM_SIDEBAR_CREATE_TABLE 6005
#define IDM_SIDEBAR_ALTER_TABLE  6006

static Sidebar *g_sidebar = NULL;
static ContentArea *g_content = NULL;
static ConnectionStore *g_store = NULL;

typedef struct {
  BOOL success;
  char error_msg[256];
} Win32DdlStatusCtx;

static void dummy_header_cb(void *user_data, int col_idx, const char *col_name) {
  (void)user_data; (void)col_idx; (void)col_name;
}
static void dummy_row_cb(void *user_data, int row_idx, int col_idx, const char *value) {
  (void)user_data; (void)row_idx; (void)col_idx; (void)value;
}
static void ddl_status_cb(void *user_data, const char *status_msg) {
  Win32DdlStatusCtx *ctx = (Win32DdlStatusCtx *)user_data;
  if (status_msg && strncmp(status_msg, "OK:", 3) == 0) {
    ctx->success = TRUE;
  } else {
    ctx->success = FALSE;
    strncpy(ctx->error_msg, status_msg ? status_msg : "Unknown error", sizeof(ctx->error_msg) - 1);
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CREATE: {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

    g_store = connection_store_new();
    if (g_store) {
      connection_store_load(g_store);
    }
    HMENU hMenu = create_app_menu();
    SetMenu(hwnd, hMenu);
    g_sidebar = create_sidebar(hwnd, hInst, g_store);
    g_content = create_content_area(hwnd, hInst);
    content_attach_sidebar(g_content, g_sidebar);

    if (g_sidebar) {
      load_saved_connections_to_tree(g_sidebar);
    }
    break;
  }
  case WM_SIZE: {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);

    int sidebar_width = 220;
    if (g_sidebar) {
      resize_sidebar(g_sidebar, 0, 0, sidebar_width, height);
    }
    if (g_content) {
      resize_content_area(g_content, sidebar_width, 0, width - sidebar_width,
                          height);
    }
    break;
  }
  case WM_COMMAND: {
    int wmId = LOWORD(wParam);
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrA(hwnd, GWLP_HINSTANCE);

    switch (wmId) {
    case IDM_FILE_EXIT:
      DestroyWindow(hwnd);
      break;
    case IDM_DB_CONNECT:
    case IDC_BTN_CREATE_CONN:
      show_new_connection_dialog(hwnd, hInst, g_sidebar);
      break;
    case IDC_BTN_RUN:
      if (g_content) {
        run_sql_query(g_content);
      }
      break;
    case IDM_HELP_ABOUT:
      MessageBoxA(hwnd,
                  "DBmrap Database Client\nVersion 1.0 (Windows Native "
                  "API)\n\nDeveloped in C By Pram With Love ^^.",
                  "About DBmrap", MB_OK | MB_ICONINFORMATION);
      break;
    case IDM_HELP_DOCS:
      MessageBoxA(hwnd, "Documentation can be found on our Github repository.",
                  "Documentation", MB_OK | MB_ICONINFORMATION);
      break;
    case IDM_SIDEBAR_DISCONNECT:
      if (g_sidebar && g_sidebar->right_click_data) {
        connection_store_remove(g_store, g_sidebar->right_click_data->conn_id);
        connection_store_save(g_store);
        SendMessageA(g_sidebar->hwndTreeView, TVM_DELETEITEM, 0, (LPARAM)g_sidebar->hRightClickItem);
      }
      break;
    case IDM_SIDEBAR_CREATE_DB:
      if (g_sidebar && g_sidebar->right_click_data) {
        show_create_database_dialog(hwnd, hInst, g_sidebar);
      }
      break;
    case IDM_SIDEBAR_DROP_DB:
      if (g_sidebar && g_sidebar->right_click_data) {
        char confirm_msg[256];
        snprintf(confirm_msg, sizeof(confirm_msg), "Are you sure you want to drop database '%s'?", g_sidebar->right_click_label);
        if (MessageBoxA(hwnd, confirm_msg, "Confirm Drop Database", MB_YESNO | MB_ICONQUESTION) == IDYES) {
          ConnectionInfo *info = connection_store_get(g_store, g_sidebar->right_click_data->conn_id);
          if (info) {
            char query[256];
            snprintf(query, sizeof(query), "DROP DATABASE %s;", g_sidebar->right_click_label);
            Win32DdlStatusCtx status_ctx = {FALSE, ""};
            if (info->type == DB_TYPE_POSTGRESQL) {
              pg_run_query(info, g_sidebar->right_click_data->db_name, query, dummy_header_cb, dummy_row_cb, ddl_status_cb, &status_ctx);
            } else if (info->type == DB_TYPE_MYSQL) {
              mysql_run_query(info, g_sidebar->right_click_data->db_name, query, dummy_header_cb, dummy_row_cb, ddl_status_cb, &status_ctx);
            }
            if (status_ctx.success) {
              sidebar_refresh_connection_by_id(g_sidebar, info->id);
            } else {
              MessageBoxA(hwnd, status_ctx.error_msg, "Error", MB_OK | MB_ICONERROR);
            }
          }
        }
      }
      break;
    case IDM_SIDEBAR_SELECT_100:
      if (g_sidebar && g_sidebar->right_click_data && g_content) {
        char query[512];
        ConnectionInfo *info = connection_store_get(g_store, g_sidebar->right_click_data->conn_id);
        if (info) {
          if (info->type == DB_TYPE_POSTGRESQL) {
            const char *schema = g_sidebar->right_click_data->schema_name ? g_sidebar->right_click_data->schema_name : "public";
            snprintf(query, sizeof(query), "SELECT * FROM \"%s\".\"%s\" LIMIT 100;", schema, g_sidebar->right_click_label);
          } else if (info->type == DB_TYPE_MYSQL) {
            snprintf(query, sizeof(query), "SELECT * FROM `%s` LIMIT 100;", g_sidebar->right_click_label);
          } else {
            snprintf(query, sizeof(query), "SELECT * FROM \"%s\" LIMIT 100;", g_sidebar->right_click_label);
          }
          content_area_set_query(g_content, g_sidebar->right_click_data->conn_id, g_sidebar->right_click_data->db_name, query, TRUE);
        }
      }
      break;
    case IDM_SIDEBAR_CREATE_TABLE:
      if (g_sidebar && g_sidebar->right_click_data && g_content) {
        ConnectionInfo *info = connection_store_get(g_store, g_sidebar->right_click_data->conn_id);
        if (info) {
          const char *query = "";
          if (info->type == DB_TYPE_POSTGRESQL) {
            query = "CREATE TABLE new_table (\n  id SERIAL PRIMARY KEY,\n  name VARCHAR(100)\n);";
          } else if (info->type == DB_TYPE_MYSQL) {
            query = "CREATE TABLE new_table (\n  id INT AUTO_INCREMENT PRIMARY KEY,\n  name VARCHAR(100)\n);";
          } else {
            query = "CREATE TABLE new_table (\n  id INTEGER PRIMARY KEY AUTOINCREMENT,\n  name TEXT\n);";
          }
          content_area_set_query(g_content, g_sidebar->right_click_data->conn_id, g_sidebar->right_click_data->db_name, query, FALSE);
        }
      }
      break;
    case IDM_SIDEBAR_ALTER_TABLE:
      if (g_sidebar && g_sidebar->right_click_data && g_content) {
        ConnectionInfo *info = connection_store_get(g_store, g_sidebar->right_click_data->conn_id);
        if (info) {
          content_area_show_table_designer(g_content, g_sidebar->right_click_data->conn_id,
                                           g_sidebar->right_click_data->db_name,
                                           g_sidebar->right_click_data->schema_name,
                                           g_sidebar->right_click_label, info);
        }
      }
      break;
    case IDC_BTN_ADD_COL:
      if (g_content) {
        extern void on_add_column_clicked(ContentArea *area);
        on_add_column_clicked(g_content);
      }
      break;
    case IDC_BTN_DROP_COL:
      if (g_content) {
        extern void on_drop_column_clicked(ContentArea *area);
        on_drop_column_clicked(g_content);
      }
      break;
    default:
      break;
    }
    break;
  }
  case WM_NOTIFY: {
    LPNMHDR lpnmhdr = (LPNMHDR)lParam;
    if (lpnmhdr->code == TVN_SELCHANGED) {
      LPNMTREEVIEW lpnmTreeView = (LPNMTREEVIEW)lParam;
      TreeNodeData *data = (TreeNodeData *)lpnmTreeView->itemNew.lParam;
      if (data && g_content) {
        // Switch to SQL Editor tab on select (Page 0)
        TabCtrl_SetCurSel(g_content->hwndTab, 0);
        NMHDR nm = {0};
        nm.hwndFrom = g_content->hwndTab;
        nm.idFrom = IDC_TAB_CONTROL;
        nm.code = TCN_SELCHANGE;
        SendMessageA(hwnd, WM_NOTIFY, (WPARAM)IDC_TAB_CONTROL, (LPARAM)&nm);

        if (data->kind == NODE_CONNECTION || data->kind == NODE_DATABASE ||
            data->kind == NODE_SCHEMA || data->kind == NODE_TABLE ||
            data->kind == NODE_COLUMN) {
          apply_connection_context(g_content, data->conn_id, data->db_name);
        } else {
          apply_connection_context(g_content, NULL, NULL);
        }
      }
    } else if (lpnmhdr->code == TVN_DELETEITEM) {
      LPNMTREEVIEW lpnmTreeView = (LPNMTREEVIEW)lParam;
      TreeNodeData *data = (TreeNodeData *)lpnmTreeView->itemOld.lParam;
      if (data) {
        free_tree_node_data(data);
      }
    } else if (lpnmhdr->idFrom == IDC_TREEVIEW && lpnmhdr->code == NM_RCLICK) {
      POINT pt;
      GetCursorPos(&pt);
      POINT ptClient = pt;
      ScreenToClient(g_sidebar->hwndTreeView, &ptClient);

      TVHITTESTINFO tvht = {0};
      tvht.pt = ptClient;
      SendMessageA(g_sidebar->hwndTreeView, TVM_HITTEST, 0, (LPARAM)&tvht);

      if (tvht.flags & (TVHT_ONITEM | TVHT_ONITEMBUTTON)) {
        SendMessageA(g_sidebar->hwndTreeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)tvht.hItem);

        TVITEMA tvi = {0};
        tvi.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_TEXT;
        tvi.hItem = tvht.hItem;
        char item_text[256] = {0};
        tvi.pszText = item_text;
        tvi.cchTextMax = sizeof(item_text);
        SendMessageA(g_sidebar->hwndTreeView, TVM_GETITEMA, 0, (LPARAM)&tvi);

        TreeNodeData *data = (TreeNodeData *)tvi.lParam;
        if (data) {
          HMENU hPopupMenu = CreatePopupMenu();
          if (data->kind == NODE_CONNECTION) {
            ConnectionInfo *info = connection_store_get(g_sidebar->conn_store, data->conn_id);
            if (info && info->type != DB_TYPE_SQLITE) {
              AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_DISCONNECT, "Disconnect");
              AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_CREATE_DB, "Create Database");
            } else {
              AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_DISCONNECT, "Disconnect");
            }
          } else if (data->kind == NODE_DATABASE || data->kind == NODE_SCHEMA) {
            ConnectionInfo *info = connection_store_get(g_sidebar->conn_store, data->conn_id);
            if (info && info->type != DB_TYPE_SQLITE && data->kind == NODE_DATABASE) {
              AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_DROP_DB, "Drop Database");
            }
            AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_CREATE_TABLE, "Create Table");
          } else if (data->kind == NODE_TABLE) {
            AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_SELECT_100, "Select 100 Row");
            AppendMenuA(hPopupMenu, MF_STRING, IDM_SIDEBAR_ALTER_TABLE, "Alter Table");
          }

          if (GetMenuItemCount(hPopupMenu) > 0) {
            g_sidebar->hRightClickItem = tvht.hItem;
            g_sidebar->right_click_data = data;
            strncpy(g_sidebar->right_click_label, item_text, sizeof(g_sidebar->right_click_label) - 1);
            TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
          }
          DestroyMenu(hPopupMenu);
        }
      }
    } else if (lpnmhdr->code == TCN_SELCHANGE) {
      if (g_content) {
        int sel = TabCtrl_GetCurSel(g_content->hwndTab);
        if (sel == 0) {
          // SQL Editor
          ShowWindow(g_content->hwndTitle, SW_SHOW);
          ShowWindow(g_content->hwndRun, SW_SHOW);
          ShowWindow(g_content->hwndEditor, SW_SHOW);
          ShowWindow(g_content->hwndStatus, SW_SHOW);
          ShowWindow(g_content->hwndResultGrid, SW_SHOW);

          // Hide Table Designer
          ShowWindow(g_content->hwndDesignerTitle, SW_HIDE);
          ShowWindow(g_content->hwndDesignerGrid, SW_HIDE);
          ShowWindow(g_content->hwndAddName, SW_HIDE);
          ShowWindow(g_content->hwndAddType, SW_HIDE);
          ShowWindow(g_content->hwndBtnAdd, SW_HIDE);
          ShowWindow(g_content->hwndBtnDrop, SW_HIDE);
        } else {
          // Hide SQL Editor
          ShowWindow(g_content->hwndTitle, SW_HIDE);
          ShowWindow(g_content->hwndRun, SW_HIDE);
          ShowWindow(g_content->hwndEditor, SW_HIDE);
          ShowWindow(g_content->hwndStatus, SW_HIDE);
          ShowWindow(g_content->hwndResultGrid, SW_HIDE);

          // Show Table Designer
          ShowWindow(g_content->hwndDesignerTitle, SW_SHOW);
          ShowWindow(g_content->hwndDesignerGrid, SW_SHOW);
          ShowWindow(g_content->hwndAddName, SW_SHOW);
          ShowWindow(g_content->hwndAddType, SW_SHOW);
          ShowWindow(g_content->hwndBtnAdd, SW_SHOW);
          ShowWindow(g_content->hwndBtnDrop, SW_SHOW);

          // Refresh Table Designer
          extern void refresh_table_designer(ContentArea *area);
          refresh_table_designer(g_content);
        }
      }
    }
    break;
  }
  case WM_DESTROY: {
    if (g_store) {
      connection_store_free(g_store);
    }
    postgres_cleanup();
    mysql_cleanup_lib();
    sqlite_cleanup_lib();
    PostQuitMessage(0);
    break;
  }
  default:
    return DefWindowProcA(hwnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  (void)hPrevInstance;
  (void)lpCmdLine;
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = "DBMRAP_MAIN";
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassExA(&wc)) {
    MessageBoxA(NULL, "Window Registration Failed!", "Error!",
                MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  HWND hwnd = CreateWindowExA(
      WS_EX_CLIENTEDGE, "DBMRAP_MAIN", "DBmrap Multi-Platform Database Client",
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, CW_USEDEFAULT,
      CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

  if (hwnd == NULL) {
    MessageBoxA(NULL, "Window Creation Failed!", "Error!",
                MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  MSG Msg;
  while (GetMessage(&Msg, NULL, 0, 0) > 0) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  return Msg.wParam;
}
