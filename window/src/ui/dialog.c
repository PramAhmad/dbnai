#include "dialog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IDC_BTN_CONNECT 6001 // Connect button
#define IDC_BTN_CANCEL 6002  // Cancel button
#define IDC_BTN_BROWSE 6003  // Browse button

#define IDC_BTN_POSTGRES 7001 // Postgres button
#define IDC_BTN_MYSQL 7002    // Mysql button
#define IDC_BTN_SQLITE 7003   // Sqlite button

typedef struct {
  HWND hwndParent;
  HINSTANCE hInst;
  Sidebar *sidebar;
  DbType db_type;

  HWND hwndDlg;
  HWND hwndName;
  HWND hwndHost;
  HWND hwndPort;
  HWND hwndUsername;
  HWND hwndPassword;
  HWND hwndDatabase;
} DialogContext;

static void CenterWindow(HWND hwnd, HWND hwndParent) {
  RECT rc, rcParent;
  GetWindowRect(hwnd, &rc);
  GetWindowRect(hwndParent, &rcParent);
  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;
  int x = rcParent.left + (rcParent.right - rcParent.left - width) / 2;
  int y = rcParent.top + (rcParent.bottom - rcParent.top - height) / 2;
  SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK DialogWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam) {
  DialogContext *ctx = (DialogContext *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

  switch (msg) {
  case WM_CREATE: {
    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
    ctx = (DialogContext *)cs->lpCreateParams;
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
    ctx->hwndDlg = hwnd;

    int y = 10;
    // Connection Name
    CreateWindowExA(0, "STATIC", "Connection Name:", WS_CHILD | WS_VISIBLE, 10,
                    y, 120, 20, hwnd, NULL, cs->hInstance, NULL);
    ctx->hwndName = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        140, y, 230, 20, hwnd, NULL, cs->hInstance, NULL);
    y += 30;

    if (ctx->db_type == DB_TYPE_SQLITE) {
      // Database Path (SQLite)
      CreateWindowExA(0, "STATIC", "Database Path:", WS_CHILD | WS_VISIBLE, 10,
                      y, 120, 20, hwnd, NULL, cs->hInstance, NULL);
      ctx->hwndDatabase = CreateWindowExA(
          WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
          140, y, 150, 20, hwnd, NULL, cs->hInstance, NULL);
      CreateWindowExA(0, "BUTTON", "Browse...",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 300, y, 70, 20,
                      hwnd, (HMENU)IDC_BTN_BROWSE, cs->hInstance, NULL);
      y += 30;
    } else {
      // Host
      CreateWindowExA(0, "STATIC", "Host:", WS_CHILD | WS_VISIBLE, 10, y, 120,
                      20, hwnd, NULL, cs->hInstance, NULL);
      ctx->hwndHost = CreateWindowExA(
          WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
          140, y, 230, 20, hwnd, NULL, cs->hInstance, NULL);
      y += 30;

      // Port
      CreateWindowExA(0, "STATIC", "Port:", WS_CHILD | WS_VISIBLE, 10, y, 120,
                      20, hwnd, NULL, cs->hInstance, NULL);
      const char *default_port =
          (ctx->db_type == DB_TYPE_POSTGRESQL) ? "5432" : "3306";
      ctx->hwndPort =
          CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", default_port,
                          WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 140, y, 230,
                          20, hwnd, NULL, cs->hInstance, NULL);
      y += 30;

      // Username
      CreateWindowExA(0, "STATIC", "Username:", WS_CHILD | WS_VISIBLE, 10, y,
                      120, 20, hwnd, NULL, cs->hInstance, NULL);
      ctx->hwndUsername = CreateWindowExA(
          WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
          140, y, 230, 20, hwnd, NULL, cs->hInstance, NULL);
      y += 30;

      // Password
      CreateWindowExA(0, "STATIC", "Password:", WS_CHILD | WS_VISIBLE, 10, y,
                      120, 20, hwnd, NULL, cs->hInstance, NULL);
      ctx->hwndPassword =
          CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                          WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL,
                          140, y, 230, 20, hwnd, NULL, cs->hInstance, NULL);
      y += 30;

      // Database
      CreateWindowExA(0, "STATIC", "Database Name:", WS_CHILD | WS_VISIBLE, 10,
                      y, 120, 20, hwnd, NULL, cs->hInstance, NULL);
      ctx->hwndDatabase = CreateWindowExA(
          WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
          140, y, 230, 20, hwnd, NULL, cs->hInstance, NULL);
      y += 30;
    }

    // Buttons
    CreateWindowExA(0, "BUTTON", "Connect",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, y, 90, 30, hwnd,
                    (HMENU)IDC_BTN_CONNECT, cs->hInstance, NULL);
    CreateWindowExA(0, "BUTTON", "Cancel",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, y, 90, 30, hwnd,
                    (HMENU)IDC_BTN_CANCEL, cs->hInstance, NULL);
    break;
  }
  case WM_COMMAND: {
    int wmId = LOWORD(wParam);
    if (wmId == IDC_BTN_CANCEL) {
      DestroyWindow(hwnd);
      return 0;
    }
    if (wmId == IDC_BTN_BROWSE) {
      OPENFILENAMEA ofn = {0};
      char szFile[MAX_PATH] = {0};
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = hwnd;
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = sizeof(szFile);
      ofn.lpstrFilter = "SQLite Databases "
                        "(*.db;*.sqlite;*.sqlite3)\0*.db;*.sqlite;*."
                        "sqlite3\0All Files (*.*)\0*.*\0";
      ofn.nFilterIndex = 1;
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
      if (GetOpenFileNameA(&ofn)) {
        SetWindowTextA(ctx->hwndDatabase, ofn.lpstrFile);
      }
      return 0;
    }
    if (wmId == IDC_BTN_CONNECT) {
      char name[128], host[128] = "", port[64] = "", username[128] = "",
                      password[128] = "", database[MAX_PATH] = "";
      GetWindowTextA(ctx->hwndName, name, sizeof(name));

      if (ctx->db_type == DB_TYPE_SQLITE) {
        GetWindowTextA(ctx->hwndDatabase, database, sizeof(database));
        if (strlen(name) == 0 || strlen(database) == 0) {
          MessageBoxA(hwnd, "Connection Name and Database Path are required.",
                      "Error", MB_OK | MB_ICONERROR);
          return 0;
        }
      } else {
        GetWindowTextA(ctx->hwndHost, host, sizeof(host));
        GetWindowTextA(ctx->hwndPort, port, sizeof(port));
        GetWindowTextA(ctx->hwndUsername, username, sizeof(username));
        GetWindowTextA(ctx->hwndPassword, password, sizeof(password));
        GetWindowTextA(ctx->hwndDatabase, database, sizeof(database));
        if (strlen(name) == 0 || strlen(host) == 0 || strlen(port) == 0 ||
            strlen(username) == 0) {
          MessageBoxA(hwnd,
                      "Connection Name, Host, Port, and Username are required.",
                      "Error", MB_OK | MB_ICONERROR);
          return 0;
        }
      }

      ConnectionInfo *info = connection_info_new(name, ctx->db_type, host, port,
                                                 username, password, database);
      if (info) {
        connection_store_add(ctx->sidebar->conn_store, info);
        connection_store_save(ctx->sidebar->conn_store);
        add_connection_node(ctx->sidebar, info);
      }

      DestroyWindow(hwnd);
      return 0;
    }
    break;
  }
  case WM_DESTROY: {
    HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    if (hwndOwner) {
      EnableWindow(hwndOwner, TRUE);
      SetFocus(hwndOwner);
    }
    free(ctx);
    break;
  }
  }
  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SelectionWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam) {
  DialogContext *ctx = (DialogContext *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

  switch (msg) {
  case WM_CREATE: {
    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
    ctx = (DialogContext *)cs->lpCreateParams;
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);

    CreateWindowExA(0, "BUTTON", "PostgreSQL",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 20, 200, 30,
                    hwnd, (HMENU)IDC_BTN_POSTGRES, cs->hInstance, NULL);
    CreateWindowExA(0, "BUTTON", "MySQL", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    20, 60, 200, 30, hwnd, (HMENU)IDC_BTN_MYSQL, cs->hInstance,
                    NULL);
    CreateWindowExA(0, "BUTTON", "SQLite",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 100, 200, 30,
                    hwnd, (HMENU)IDC_BTN_SQLITE, cs->hInstance, NULL);
    CreateWindowExA(0, "BUTTON", "Cancel",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 140, 200, 30,
                    hwnd, (HMENU)IDC_BTN_CANCEL, cs->hInstance, NULL);
    break;
  }
  case WM_COMMAND: {
    int wmId = LOWORD(wParam);
    if (wmId == IDC_BTN_CANCEL) {
      DestroyWindow(hwnd);
      return 0;
    }
    DbType selected_type;
    BOOL selected = FALSE;
    if (wmId == IDC_BTN_POSTGRES) {
      selected_type = DB_TYPE_POSTGRESQL;
      selected = TRUE;
    } else if (wmId == IDC_BTN_MYSQL) {
      selected_type = DB_TYPE_MYSQL;
      selected = TRUE;
    } else if (wmId == IDC_BTN_SQLITE) {
      selected_type = DB_TYPE_SQLITE;
      selected = TRUE;
    }

    if (selected) {
      HWND hwndParent = GetWindow(hwnd, GW_OWNER);
      HINSTANCE hInst = ctx->hInst;
      Sidebar *sidebar = ctx->sidebar;

      DestroyWindow(hwnd);

      DialogContext *form_ctx = malloc(sizeof(DialogContext));
      if (!form_ctx)
        return 0;
      form_ctx->hwndParent = hwndParent;
      form_ctx->hInst = hInst;
      form_ctx->sidebar = sidebar;
      form_ctx->db_type = selected_type;

      WNDCLASSEXA wc = {0};
      wc.cbSize = sizeof(WNDCLASSEXA);
      wc.lpfnWndProc = DialogWndProc;
      wc.hInstance = hInst;
      wc.hCursor = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
      wc.lpszClassName = "DBMRAP_FORM_DIALOG";
      RegisterClassExA(&wc);

      int height = (selected_type == DB_TYPE_SQLITE) ? 140 : 260;

      EnableWindow(hwndParent, FALSE);

      HWND hwndForm = CreateWindowExA(
          WS_EX_DLGMODALFRAME, "DBMRAP_FORM_DIALOG",
          (selected_type == DB_TYPE_POSTGRESQL) ? "PostgreSQL Connection"
          : (selected_type == DB_TYPE_MYSQL)    ? "MySQL Connection"
                                                : "SQLite Connection",
          WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT,
          CW_USEDEFAULT, 400, height, hwndParent, NULL, hInst, form_ctx);
      CenterWindow(hwndForm, hwndParent);
    }
    break;
  }
  case WM_DESTROY: {
    HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    if (hwndOwner && !IsWindowEnabled(hwndOwner)) {
      EnableWindow(hwndOwner, TRUE);
      SetFocus(hwndOwner);
    }
    free(ctx);
    break;
  }
  }
  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void show_new_connection_dialog(HWND hwndParent, HINSTANCE hInst,
                                Sidebar *sidebar) {
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.lpfnWndProc = SelectionWndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = "DBMRAP_SEL_DIALOG";
  RegisterClassExA(&wc);

  DialogContext *ctx = malloc(sizeof(DialogContext));
  if (!ctx)
    return;
  ctx->hwndParent = hwndParent;
  ctx->hInst = hInst;
  ctx->sidebar = sidebar;

  EnableWindow(hwndParent, FALSE);

  HWND hwndSel = CreateWindowExA(
      WS_EX_DLGMODALFRAME, "DBMRAP_SEL_DIALOG", "New Connection",
      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT,
      CW_USEDEFAULT, 256, 220, hwndParent, NULL, hInst, ctx);
  CenterWindow(hwndSel, hwndParent);
}

typedef struct {
  HWND hwndParent;
  HINSTANCE hInst;
  Sidebar *sidebar;
  HWND hwndDlg;
  HWND hwndDbName;
} CreateDbCtx;

#define IDC_BTN_OK 8001
#define IDC_EDT_DBNAME 8002

static void dummy_header_cb(void *user_data, int col_idx, const char *col_name) {
  (void)user_data; (void)col_idx; (void)col_name;
}
static void dummy_row_cb(void *user_data, int row_idx, int col_idx, const char *value) {
  (void)user_data; (void)row_idx; (void)col_idx; (void)value;
}

typedef struct {
  BOOL success;
  char error_msg[256];
} Win32DdlStatusCtx;

static void ddl_status_cb(void *user_data, const char *status_msg) {
  Win32DdlStatusCtx *ctx = (Win32DdlStatusCtx *)user_data;
  if (status_msg && strncmp(status_msg, "OK:", 3) == 0) {
    ctx->success = TRUE;
  } else {
    ctx->success = FALSE;
    strncpy(ctx->error_msg, status_msg ? status_msg : "Unknown error", sizeof(ctx->error_msg) - 1);
  }
}

static BOOL CALLBACK SetFontProc(HWND hwnd, LPARAM lParam) {
  SendMessageA(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
  return TRUE;
}

LRESULT CALLBACK CreateDbWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  CreateDbCtx *ctx = (CreateDbCtx *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
  switch (msg) {
  case WM_CREATE: {
    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
    ctx = (CreateDbCtx *)cs->lpCreateParams;
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
    ctx->hwndDlg = hwnd;

    CreateWindowExA(0, "STATIC", "Database Name:", WS_CHILD | WS_VISIBLE, 10, 15, 120, 20, hwnd, NULL, cs->hInstance, NULL);
    ctx->hwndDbName = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 140, 15, 180, 20, hwnd, (HMENU)IDC_EDT_DBNAME, cs->hInstance, NULL);

    CreateWindowExA(0, "BUTTON", "Create", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 130, 50, 90, 30, hwnd, (HMENU)IDC_BTN_OK, cs->hInstance, NULL);
    CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 230, 50, 90, 30, hwnd, (HMENU)IDC_BTN_CANCEL, cs->hInstance, NULL);

    HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);
    if (hFont) {
      EnumChildWindows(hwnd, (WNDENUMPROC)SetFontProc, (LPARAM)hFont);
    }
    break;
  }
  case WM_COMMAND: {
    int wmId = LOWORD(wParam);
    if (wmId == IDC_BTN_CANCEL) {
      DestroyWindow(hwnd);
      return 0;
    }
    if (wmId == IDC_BTN_OK) {
      char dbname[128] = {0};
      GetWindowTextA(ctx->hwndDbName, dbname, sizeof(dbname));
      if (strlen(dbname) == 0) {
        MessageBoxA(hwnd, "Database Name is required.", "Error", MB_OK | MB_ICONERROR);
        return 0;
      }

      #include "../db/postgres.h"
      #include "../db/mysql.h"
      ConnectionInfo *info = connection_store_get(ctx->sidebar->conn_store, ctx->sidebar->right_click_data->conn_id);
      if (info) {
        char query[256];
        snprintf(query, sizeof(query), "CREATE DATABASE %s;", dbname);
        Win32DdlStatusCtx status_ctx = {FALSE, ""};
        if (info->type == DB_TYPE_POSTGRESQL) {
          pg_run_query(info, ctx->sidebar->right_click_data->db_name, query, dummy_header_cb, dummy_row_cb, ddl_status_cb, &status_ctx);
        } else if (info->type == DB_TYPE_MYSQL) {
          mysql_run_query(info, ctx->sidebar->right_click_data->db_name, query, dummy_header_cb, dummy_row_cb, ddl_status_cb, &status_ctx);
        }
        if (status_ctx.success) {
          sidebar_refresh_connection_by_id(ctx->sidebar, info->id);
          DestroyWindow(hwnd);
        } else {
          MessageBoxA(hwnd, status_ctx.error_msg, "Error", MB_OK | MB_ICONERROR);
        }
      } else {
        DestroyWindow(hwnd);
      }
      return 0;
    }
    break;
  }
  case WM_DESTROY: {
    HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    if (hwndOwner) {
      EnableWindow(hwndOwner, TRUE);
      SetFocus(hwndOwner);
    }
    free(ctx);
    break;
  }
  }
  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void show_create_database_dialog(HWND hwndParent, HINSTANCE hInst, Sidebar *sidebar) {
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.lpfnWndProc = CreateDbWndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = "DBMRAP_CREATEDB_DIALOG";
  RegisterClassExA(&wc);

  CreateDbCtx *ctx = malloc(sizeof(CreateDbCtx));
  if (!ctx)
    return;
  ctx->hwndParent = hwndParent;
  ctx->hInst = hInst;
  ctx->sidebar = sidebar;

  EnableWindow(hwndParent, FALSE);

  HWND hwndDlg = CreateWindowExA(
      WS_EX_DLGMODALFRAME, "DBMRAP_CREATEDB_DIALOG", "Create Database",
      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT,
      CW_USEDEFAULT, 350, 130, hwndParent, NULL, hInst, ctx);
  CenterWindow(hwndDlg, hwndParent);
}
