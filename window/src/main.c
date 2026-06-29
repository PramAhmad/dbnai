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

static Sidebar *g_sidebar = NULL;
static ContentArea *g_content = NULL;
static ConnectionStore *g_store = NULL;

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
