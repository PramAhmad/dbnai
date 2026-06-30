#include "content.h"
#include "../db/mysql.h"
#include "../db/postgres.h"
#include "../db/sqlite.h"
#include "sidebar.h"
#include "store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *conn_id;
  char *query_text;
} SavedQuery;

static SavedQuery *g_queries = NULL;
static int g_query_count = 0;

static void save_query(const char *conn_id, const char *text) {
  if (!conn_id)
    return;
  for (int i = 0; i < g_query_count; i++) {
    if (strcmp(g_queries[i].conn_id, conn_id) == 0) {
      free(g_queries[i].query_text);
      g_queries[i].query_text = strdup(text ? text : "");
      return;
    }
  }
  g_queries = realloc(g_queries, sizeof(SavedQuery) * (g_query_count + 1));
  g_queries[g_query_count].conn_id = strdup(conn_id);
  g_queries[g_query_count].query_text = strdup(text ? text : "");
  g_query_count++;
}

static const char *get_saved_query(const char *conn_id) {
  if (!conn_id)
    return NULL;
  for (int i = 0; i < g_query_count; i++) {
    if (strcmp(g_queries[i].conn_id, conn_id) == 0) {
      return g_queries[i].query_text;
    }
  }
  return NULL;
}

static LRESULT CALLBACK EditorSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                           LPARAM lParam, UINT_PTR uIdSubclass,
                                           DWORD_PTR dwRefData) {
  (void)uIdSubclass;
  if (uMsg == WM_CHAR && wParam == 10) { // Ctrl+Enter produces LF (10)
    ContentArea *area = (ContentArea *)dwRefData;
    run_sql_query(area);
    return 0;
  }
  if (uMsg == WM_KEYDOWN && wParam == VK_RETURN &&
      (GetKeyState(VK_CONTROL) & 0x8000)) {
    ContentArea *area = (ContentArea *)dwRefData;
    run_sql_query(area);
    return 0;
  }
  return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK ContentWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam) {
  if (msg == WM_COMMAND || msg == WM_NOTIFY) {
    return SendMessageA(GetParent(hwnd), msg, wParam, lParam);
  }
  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

ContentArea *create_content_area(HWND hwndParent, HINSTANCE hInst) {
  ContentArea *area = malloc(sizeof(ContentArea));
  if (!area)
    return NULL;

  area->sidebar = NULL;
  area->active_conn_id = NULL;
  area->active_db = NULL;
  area->designer_conn_id = NULL;
  area->designer_db_name = NULL;
  area->designer_schema_name = NULL;
  area->designer_table_name = NULL;

  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.lpfnWndProc = ContentWndProc;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = "DBMRAP_CONTENT";
  RegisterClassExA(&wc);

  area->hwndContainer = CreateWindowExA(
      0, "DBMRAP_CONTENT", "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0,
      0, hwndParent, NULL, hInst, NULL);

  // Tab Control
  area->hwndTab = CreateWindowExA(
      0, WC_TABCONTROLA, "", WS_CHILD | WS_VISIBLE | TCS_TABS,
      0, 0, 0, 0, area->hwndContainer, (HMENU)IDC_TAB_CONTROL, hInst, NULL);

  TCITEMA tie = {0};
  tie.mask = TCIF_TEXT;
  tie.pszText = "SQL Editor";
  TabCtrl_InsertItem(area->hwndTab, 0, &tie);
  tie.pszText = "Table Designer";
  TabCtrl_InsertItem(area->hwndTab, 1, &tie);

  // SQL Editor controls
  area->hwndTitle =
      CreateWindowExA(0, "STATIC", "SQL Editor", WS_CHILD | WS_VISIBLE, 5, 5,
                      200, 20, area->hwndContainer, NULL, hInst, NULL);

  area->hwndRun = CreateWindowExA(
      0, "BUTTON", "Run", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
      5, 30, 80, 25, area->hwndContainer, (HMENU)IDC_BTN_RUN, hInst, NULL);

  area->hwndEditor = CreateWindowExA(
      WS_EX_CLIENTEDGE, "EDIT", "",
      WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
          WS_VSCROLL | WS_HSCROLL | ES_WANTRETURN,
      5, 60, 200, 100, area->hwndContainer, (HMENU)IDC_EDT_SQL, hInst, NULL);

  HFONT hFont =
      CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                  FIXED_PITCH | FF_MODERN, "Consolas");
  if (hFont) {
    SendMessageA(area->hwndEditor, WM_SETFONT, (WPARAM)hFont, TRUE);
  }

  SetWindowSubclass(area->hwndEditor, EditorSubclassProc, 1, (DWORD_PTR)area);

  area->hwndStatus = CreateWindowExA(
      0, "STATIC",
      "Select a connection/database from sidebar to start querying.",
      WS_CHILD | WS_VISIBLE, 5, 170, 200, 20, area->hwndContainer, NULL, hInst,
      NULL);

  area->hwndResultGrid = CreateWindowExA(
      WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
      WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, 5, 200, 200, 200,
      area->hwndContainer, (HMENU)IDC_LST_RESULT, hInst, NULL);
  ListView_SetExtendedListViewStyle(area->hwndResultGrid,
                                    LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                        LVS_EX_DOUBLEBUFFER);

  // Table Designer controls
  area->hwndDesignerTitle = CreateWindowExA(
      0, "STATIC", "Table Designer", WS_CHILD,
      5, 35, 200, 20, area->hwndContainer, NULL, hInst, NULL);

  area->hwndDesignerGrid = CreateWindowExA(
      WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
      WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, 5, 60, 200, 200,
      area->hwndContainer, (HMENU)IDC_LST_DESIGNER, hInst, NULL);
  ListView_SetExtendedListViewStyle(area->hwndDesignerGrid,
                                    LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |
                                        LVS_EX_DOUBLEBUFFER);

  area->hwndAddName = CreateWindowExA(
      WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL,
      5, 270, 120, 20, area->hwndContainer, (HMENU)IDC_EDT_ADD_NAME, hInst, NULL);

  area->hwndAddType = CreateWindowExA(
      WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL,
      130, 270, 150, 20, area->hwndContainer, (HMENU)IDC_EDT_ADD_TYPE, hInst, NULL);

  area->hwndBtnAdd = CreateWindowExA(
      0, "BUTTON", "Add Column", WS_CHILD | BS_PUSHBUTTON,
      290, 270, 90, 25, area->hwndContainer, (HMENU)IDC_BTN_ADD_COL, hInst, NULL);

  area->hwndBtnDrop = CreateWindowExA(
      0, "BUTTON", "Drop Column", WS_CHILD | BS_PUSHBUTTON,
      390, 270, 95, 25, area->hwndContainer, (HMENU)IDC_BTN_DROP_COL, hInst, NULL);

  // Set cue banners (placeholder text) for design edits
  SendMessageA(area->hwndAddName, EM_SETCUEBANNER, FALSE, (LPARAM)L"Column Name");
  SendMessageA(area->hwndAddType, EM_SETCUEBANNER, FALSE, (LPARAM)L"Data Type");

  // Apply default font
  HFONT hDefaultFont = GetStockObject(DEFAULT_GUI_FONT);
  if (hDefaultFont) {
    SendMessageA(area->hwndTab, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndTitle, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndRun, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndStatus, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndDesignerTitle, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndAddName, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndAddType, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndBtnAdd, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
    SendMessageA(area->hwndBtnDrop, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
  }

  // Hide Table Designer controls initially
  ShowWindow(area->hwndDesignerTitle, SW_HIDE);
  ShowWindow(area->hwndDesignerGrid, SW_HIDE);
  ShowWindow(area->hwndAddName, SW_HIDE);
  ShowWindow(area->hwndAddType, SW_HIDE);
  ShowWindow(area->hwndBtnAdd, SW_HIDE);
  ShowWindow(area->hwndBtnDrop, SW_HIDE);

  return area;
}

void resize_content_area(ContentArea *area, int x, int y, int width,
                         int height) {
  if (!area)
    return;

  MoveWindow(area->hwndContainer, x, y, width, height, TRUE);
  MoveWindow(area->hwndTab, 0, 0, width, height, TRUE);

  // Position SQL Editor components (starts below tab headers)
  MoveWindow(area->hwndTitle, 10, 35, width - 20, 20, TRUE);
  MoveWindow(area->hwndRun, 10, 60, 80, 25, TRUE);

  int editor_height = (height - 280) / 2;
  if (editor_height < 80)
    editor_height = 80;

  MoveWindow(area->hwndEditor, 10, 90, width - 20, editor_height, TRUE);
  MoveWindow(area->hwndStatus, 10, 90 + editor_height + 5, width - 20, 20, TRUE);

  int grid_y = 90 + editor_height + 30;
  int grid_height = height - grid_y - 15;
  if (grid_height < 50)
    grid_height = 50;

  MoveWindow(area->hwndResultGrid, 10, grid_y, width - 20, grid_height, TRUE);

  // Position Table Designer components
  MoveWindow(area->hwndDesignerTitle, 10, 35, width - 20, 20, TRUE);

  int designer_grid_height = height - 145;
  if (designer_grid_height < 100)
    designer_grid_height = 100;
  MoveWindow(area->hwndDesignerGrid, 10, 60, width - 20, designer_grid_height, TRUE);

  int bottom_y = height - 75;
  MoveWindow(area->hwndAddName, 10, bottom_y, 150, 25, TRUE);
  MoveWindow(area->hwndAddType, 170, bottom_y, 180, 25, TRUE);
  MoveWindow(area->hwndBtnAdd, 360, bottom_y, 100, 25, TRUE);
  MoveWindow(area->hwndBtnDrop, 470, bottom_y, 100, 25, TRUE);
}

void content_attach_sidebar(ContentArea *area, Sidebar *sidebar) {
  if (area) {
    area->sidebar = sidebar;
  }
}

void set_status_text(ContentArea *area, const char *text) {
  if (area && area->hwndStatus) {
    SetWindowTextA(area->hwndStatus, text ? text : "");
  }
}

void clear_result_grid(ContentArea *area) {
  if (!area)
    return;
  ListView_DeleteAllItems(area->hwndResultGrid);
  HWND hwndHeader =
      (HWND)SendMessageA(area->hwndResultGrid, LVM_GETHEADER, 0, 0);
  if (hwndHeader) {
    int col_count = (int)SendMessageA(hwndHeader, HDM_GETITEMCOUNT, 0, 0);
    for (int i = col_count - 1; i >= 0; i--) {
      SendMessageA(area->hwndResultGrid, LVM_DELETECOLUMN, i, 0);
    }
  }
}

static void query_header_callback(void *user_data, int col_idx,
                                  const char *col_name) {
  ContentArea *area = (ContentArea *)user_data;
  LVCOLUMNA lvc = {0};
  lvc.mask = LVCF_TEXT | LVCF_WIDTH;
  lvc.pszText = (LPSTR)col_name;
  lvc.cx = 120;
  SendMessageA(area->hwndResultGrid, LVM_INSERTCOLUMNA, col_idx, (LPARAM)&lvc);
}

static void query_row_callback(void *user_data, int row_idx, int col_idx,
                               const char *value) {
  ContentArea *area = (ContentArea *)user_data;
  if (col_idx == 0) {
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row_idx;
    lvi.iSubItem = 0;
    lvi.pszText = (LPSTR)(value ? value : "NULL");
    SendMessageA(area->hwndResultGrid, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
  } else {
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row_idx;
    lvi.iSubItem = col_idx;
    lvi.pszText = (LPSTR)(value ? value : "NULL");
    SendMessageA(area->hwndResultGrid, LVM_SETITEMA, 0, (LPARAM)&lvi);
  }
}

static void query_status_callback(void *user_data, const char *status_msg) {
  ContentArea *area = (ContentArea *)user_data;
  set_status_text(area, status_msg);
}

void apply_connection_context(ContentArea *area, const char *conn_id,
                              const char *db_name) {
  if (!area)
    return;

  if (area->active_conn_id) {
    int len = GetWindowTextLengthA(area->hwndEditor);
    char *buf = malloc(len + 1);
    if (buf) {
      GetWindowTextA(area->hwndEditor, buf, len + 1);
      save_query(area->active_conn_id, buf);
      free(buf);
    }
  }

  free(area->active_conn_id);
  area->active_conn_id = conn_id ? strdup(conn_id) : NULL;

  free(area->active_db);
  area->active_db = db_name ? strdup(db_name) : NULL;

  const char *saved = conn_id ? get_saved_query(conn_id) : NULL;
  SetWindowTextA(area->hwndEditor, saved ? saved : "");

  if (!conn_id) {
    SetWindowTextA(area->hwndTitle, "SQL Editor");
    EnableWindow(area->hwndRun, FALSE);
    set_status_text(
        area, "Select a connection/database from sidebar to start querying.");
    return;
  }

  ConnectionInfo *info =
      connection_store_get(area->sidebar->conn_store, conn_id);
  if (!info) {
    EnableWindow(area->hwndRun, FALSE);
    set_status_text(area, "Connection metadata not found.");
    return;
  }

  EnableWindow(area->hwndRun, TRUE);

  const char *db_type_name = "Unknown";
  if (info->type == DB_TYPE_POSTGRESQL)
    db_type_name = "PostgreSQL";
  else if (info->type == DB_TYPE_MYSQL)
    db_type_name = "MySQL";
  else if (info->type == DB_TYPE_SQLITE)
    db_type_name = "SQLite";

  const char *display_db =
      (db_name && strlen(db_name) > 0)
          ? db_name
          : ((info->database && strlen(info->database) > 0) ? info->database
                                                            : "");
  char title[256];
  if (strlen(display_db) > 0) {
    snprintf(title, sizeof(title), "SQL Editor (%s) - %s / %s", db_type_name,
             info->name, display_db);
  } else {
    snprintf(title, sizeof(title), "SQL Editor (%s) - %s", db_type_name,
             info->name);
  }
  SetWindowTextA(area->hwndTitle, title);
  set_status_text(area, "Ready. Run with button or Ctrl+Enter.");
}

#include <ctype.h>

static BOOL is_ddl_query(const char *query) {
  if (!query) return FALSE;
  const char *q = query;
  while (*q && isspace((unsigned char)*q)) {
    q++;
  }
  if (_strnicmp(q, "CREATE", 6) == 0 ||
      _strnicmp(q, "DROP", 4) == 0 ||
      _strnicmp(q, "ALTER", 5) == 0 ||
      _strnicmp(q, "RENAME", 6) == 0 ||
      _strnicmp(q, "TRUNCATE", 8) == 0) {
    return TRUE;
  }
  return FALSE;
}

void run_sql_query(ContentArea *area) {
  if (!area || !area->sidebar || !area->active_conn_id) {
    set_status_text(area, "Please select a connection first.");
    return;
  }

  ConnectionInfo *conn_info =
      connection_store_get(area->sidebar->conn_store, area->active_conn_id);
  if (!conn_info) {
    set_status_text(area, "Connection metadata not found.");
    return;
  }

  int len = GetWindowTextLengthA(area->hwndEditor);
  char *query = malloc(len + 1);
  if (!query)
    return;
  GetWindowTextA(area->hwndEditor, query, len + 1);

  if (strlen(query) == 0) {
    set_status_text(area, "Query is empty.");
    free(query);
    return;
  }

  BOOL ddl = is_ddl_query(query);

  save_query(area->active_conn_id, query);
  clear_result_grid(area);

  if (conn_info->type == DB_TYPE_POSTGRESQL) {
    pg_run_query(conn_info, area->active_db, query, query_header_callback,
                 query_row_callback, query_status_callback, area);
  } else if (conn_info->type == DB_TYPE_MYSQL) {
    mysql_run_query(conn_info, area->active_db, query, query_header_callback,
                    query_row_callback, query_status_callback, area);
  } else if (conn_info->type == DB_TYPE_SQLITE) {
    sqlite_run_query(conn_info, area->active_db, query, query_header_callback,
                     query_row_callback, query_status_callback, area);
  } else {
    set_status_text(area, "Unsupported database type.");
  }

  if (ddl) {
    sidebar_refresh_connection_by_id(area->sidebar, area->active_conn_id);
  }

  free(query);
}

static void designer_header_callback(void *user_data, int col_idx,
                                     const char *col_name) {
  (void)user_data;
  (void)col_idx;
  (void)col_name;
}

static void designer_row_callback(void *user_data, int row_idx, int col_idx,
                                  const char *value) {
  ContentArea *area = (ContentArea *)user_data;
  HWND hwndGrid = area->hwndDesignerGrid;

  if (area->designer_db_type == DB_TYPE_SQLITE) {
    if (col_idx == 1) { // name
      LVITEMA lvi = {0};
      lvi.mask = LVIF_TEXT;
      lvi.iItem = row_idx;
      lvi.iSubItem = 0;
      lvi.pszText = (LPSTR)(value ? value : "");
      SendMessageA(hwndGrid, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
    } else if (col_idx == 2) { // type
      LVITEMA lvi = {0};
      lvi.mask = LVIF_TEXT;
      lvi.iItem = row_idx;
      lvi.iSubItem = 1;
      lvi.pszText = (LPSTR)(value ? value : "");
      SendMessageA(hwndGrid, LVM_SETITEMA, 0, (LPARAM)&lvi);
    } else if (col_idx == 3) { // notnull
      LVITEMA lvi = {0};
      lvi.mask = LVIF_TEXT;
      lvi.iItem = row_idx;
      lvi.iSubItem = 2;
      lvi.pszText = (strcmp(value ? value : "0", "0") == 0) ? "YES" : "NO";
      SendMessageA(hwndGrid, LVM_SETITEMA, 0, (LPARAM)&lvi);
    }
  } else { // Postgres / MySQL
    if (col_idx == 0) { // name
      LVITEMA lvi = {0};
      lvi.mask = LVIF_TEXT;
      lvi.iItem = row_idx;
      lvi.iSubItem = 0;
      lvi.pszText = (LPSTR)(value ? value : "");
      SendMessageA(hwndGrid, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
    } else if (col_idx == 1) { // type
      LVITEMA lvi = {0};
      lvi.mask = LVIF_TEXT;
      lvi.iItem = row_idx;
      lvi.iSubItem = 1;
      lvi.pszText = (LPSTR)(value ? value : "");
      SendMessageA(hwndGrid, LVM_SETITEMA, 0, (LPARAM)&lvi);
    } else if (col_idx == 2) { // nullable
      LVITEMA lvi = {0};
      lvi.mask = LVIF_TEXT;
      lvi.iItem = row_idx;
      lvi.iSubItem = 2;
      lvi.pszText = (LPSTR)(value ? value : "");
      SendMessageA(hwndGrid, LVM_SETITEMA, 0, (LPARAM)&lvi);
    }
  }
}

static void designer_status_callback(void *user_data, const char *status_msg) {
  (void)user_data;
  (void)status_msg;
}

typedef struct {
  BOOL success;
  char error_msg[256];
} Win32DdlStatusCtx;

static void win32_ddl_status_callback(void *user_data, const char *status_msg) {
  Win32DdlStatusCtx *ctx = (Win32DdlStatusCtx *)user_data;
  if (status_msg && strncmp(status_msg, "OK:", 3) == 0) {
    ctx->success = TRUE;
  } else {
    ctx->success = FALSE;
    strncpy(ctx->error_msg, status_msg ? status_msg : "Unknown error", sizeof(ctx->error_msg) - 1);
  }
}

void refresh_table_designer(ContentArea *area) {
  if (!area || !area->designer_conn_id || !area->designer_table_name)
    return;

  ListView_DeleteAllItems(area->hwndDesignerGrid);

  HWND hwndHeader = (HWND)SendMessageA(area->hwndDesignerGrid, LVM_GETHEADER, 0, 0);
  if (hwndHeader) {
    int col_count = (int)SendMessageA(hwndHeader, HDM_GETITEMCOUNT, 0, 0);
    for (int i = col_count - 1; i >= 0; i--) {
      SendMessageA(area->hwndDesignerGrid, LVM_DELETECOLUMN, i, 0);
    }
  }

  LVCOLUMNA lvc = {0};
  lvc.mask = LVCF_TEXT | LVCF_WIDTH;
  lvc.cx = 180;
  lvc.pszText = "Column Name";
  SendMessageA(area->hwndDesignerGrid, LVM_INSERTCOLUMNA, 0, (LPARAM)&lvc);

  lvc.cx = 180;
  lvc.pszText = "Data Type";
  SendMessageA(area->hwndDesignerGrid, LVM_INSERTCOLUMNA, 1, (LPARAM)&lvc);

  lvc.cx = 100;
  lvc.pszText = "Nullable";
  SendMessageA(area->hwndDesignerGrid, LVM_INSERTCOLUMNA, 2, (LPARAM)&lvc);

  char title_text[256];
  snprintf(title_text, sizeof(title_text), "Table Designer: %s", area->designer_table_name);
  SetWindowTextA(area->hwndDesignerTitle, title_text);

  ConnectionInfo *info = connection_store_get(area->sidebar->conn_store, area->designer_conn_id);
  if (!info) return;

  char query[512];
  if (info->type == DB_TYPE_POSTGRESQL) {
    const char *schema = area->designer_schema_name ? area->designer_schema_name : "public";
    snprintf(query, sizeof(query),
             "SELECT column_name, data_type, is_nullable FROM information_schema.columns "
             "WHERE table_name = '%s' AND table_schema = '%s' ORDER BY ordinal_position;",
             area->designer_table_name, schema);
    pg_run_query(info, area->designer_db_name, query, designer_header_callback,
                 designer_row_callback, designer_status_callback, area);
  } else if (info->type == DB_TYPE_MYSQL) {
    snprintf(query, sizeof(query),
             "SELECT column_name, column_type, is_nullable FROM information_schema.columns "
             "WHERE table_name = '%s' AND table_schema = '%s' ORDER BY ordinal_position;",
             area->designer_table_name, area->designer_db_name);
    mysql_run_query(info, area->designer_db_name, query, designer_header_callback,
                    designer_row_callback, designer_status_callback, area);
  } else {
    snprintf(query, sizeof(query), "PRAGMA table_info('%s');", area->designer_table_name);
    sqlite_run_query(info, area->designer_db_name, query, designer_header_callback,
                     designer_row_callback, designer_status_callback, area);
  }
}

void on_add_column_clicked(ContentArea *area) {
  char col_name[128] = {0};
  char col_type[128] = {0};
  GetWindowTextA(area->hwndAddName, col_name, sizeof(col_name));
  GetWindowTextA(area->hwndAddType, col_type, sizeof(col_type));

  if (strlen(col_name) == 0 || strlen(col_type) == 0) {
    MessageBoxA(area->hwndContainer, "Column name and type are required.", "Error", MB_OK | MB_ICONERROR);
    return;
  }

  ConnectionInfo *info = connection_store_get(area->sidebar->conn_store, area->designer_conn_id);
  if (!info) return;

  char query[512];
  if (info->type == DB_TYPE_POSTGRESQL) {
    const char *schema = area->designer_schema_name ? area->designer_schema_name : "public";
    snprintf(query, sizeof(query), "ALTER TABLE \"%s\".\"%s\" ADD COLUMN \"%s\" %s;",
             schema, area->designer_table_name, col_name, col_type);
  } else if (info->type == DB_TYPE_MYSQL) {
    snprintf(query, sizeof(query), "ALTER TABLE `%s` ADD COLUMN `%s` %s;",
             area->designer_table_name, col_name, col_type);
  } else {
    snprintf(query, sizeof(query), "ALTER TABLE \"%s\" ADD COLUMN \"%s\" %s;",
             area->designer_table_name, col_name, col_type);
  }

  Win32DdlStatusCtx status_ctx = {FALSE, ""};
  if (info->type == DB_TYPE_POSTGRESQL) {
    pg_run_query(info, area->designer_db_name, query, designer_header_callback, designer_row_callback, win32_ddl_status_callback, &status_ctx);
  } else if (info->type == DB_TYPE_MYSQL) {
    mysql_run_query(info, area->designer_db_name, query, designer_header_callback, designer_row_callback, win32_ddl_status_callback, &status_ctx);
  } else {
    sqlite_run_query(info, area->designer_db_name, query, designer_header_callback, designer_row_callback, win32_ddl_status_callback, &status_ctx);
  }

  if (status_ctx.success) {
    SetWindowTextA(area->hwndAddName, "");
    SetWindowTextA(area->hwndAddType, "");
    sidebar_refresh_connection_by_id(area->sidebar, area->designer_conn_id);
    refresh_table_designer(area);
  } else {
    MessageBoxA(area->hwndContainer, status_ctx.error_msg, "Error Executing ALTER TABLE", MB_OK | MB_ICONERROR);
  }
}

void on_drop_column_clicked(ContentArea *area) {
  int sel = ListView_GetNextItem(area->hwndDesignerGrid, -1, LVNI_SELECTED);
  if (sel == -1) {
    MessageBoxA(area->hwndContainer, "Please select a column to drop.", "Error", MB_OK | MB_ICONWARNING);
    return;
  }

  char col_name[128] = {0};
  ListView_GetItemText(area->hwndDesignerGrid, sel, 0, col_name, sizeof(col_name));

  if (strlen(col_name) == 0) return;

  char confirm_msg[256];
  snprintf(confirm_msg, sizeof(confirm_msg), "Are you sure you want to drop column '%s'?", col_name);
  if (MessageBoxA(area->hwndContainer, confirm_msg, "Confirm Drop Column", MB_YESNO | MB_ICONQUESTION) != IDYES) {
    return;
  }

  ConnectionInfo *info = connection_store_get(area->sidebar->conn_store, area->designer_conn_id);
  if (!info) return;

  char query[512];
  if (info->type == DB_TYPE_POSTGRESQL) {
    const char *schema = area->designer_schema_name ? area->designer_schema_name : "public";
    snprintf(query, sizeof(query), "ALTER TABLE \"%s\".\"%s\" DROP COLUMN \"%s\";",
             schema, area->designer_table_name, col_name);
  } else if (info->type == DB_TYPE_MYSQL) {
    snprintf(query, sizeof(query), "ALTER TABLE `%s` DROP COLUMN `%s`;",
             area->designer_table_name, col_name);
  } else {
    snprintf(query, sizeof(query), "ALTER TABLE \"%s\" DROP COLUMN \"%s\";",
             area->designer_table_name, col_name);
  }

  Win32DdlStatusCtx status_ctx = {FALSE, ""};
  if (info->type == DB_TYPE_POSTGRESQL) {
    pg_run_query(info, area->designer_db_name, query, designer_header_callback, designer_row_callback, win32_ddl_status_callback, &status_ctx);
  } else if (info->type == DB_TYPE_MYSQL) {
    mysql_run_query(info, area->designer_db_name, query, designer_header_callback, designer_row_callback, win32_ddl_status_callback, &status_ctx);
  } else {
    sqlite_run_query(info, area->designer_db_name, query, designer_header_callback, designer_row_callback, win32_ddl_status_callback, &status_ctx);
  }

  if (status_ctx.success) {
    sidebar_refresh_connection_by_id(area->sidebar, area->designer_conn_id);
    refresh_table_designer(area);
  } else {
    MessageBoxA(area->hwndContainer, status_ctx.error_msg, "Error Executing ALTER TABLE", MB_OK | MB_ICONERROR);
  }
}

void content_area_set_query(ContentArea *area, const char *conn_id,
                            const char *db_name, const char *query,
                            BOOL execute) {
  if (!area)
    return;

  apply_connection_context(area, conn_id, db_name);

  if (query) {
    SetWindowTextA(area->hwndEditor, query);
    if (execute) {
      run_sql_query(area);
    }
  }
}

void content_area_show_table_designer(ContentArea *area, const char *conn_id,
                                      const char *db_name, const char *schema_name,
                                      const char *table_name, ConnectionInfo *info) {
  if (!area)
    return;

  free(area->designer_conn_id);
  area->designer_conn_id = conn_id ? strdup(conn_id) : NULL;

  free(area->designer_db_name);
  area->designer_db_name = db_name ? strdup(db_name) : NULL;

  free(area->designer_schema_name);
  area->designer_schema_name = schema_name ? strdup(schema_name) : NULL;

  free(area->designer_table_name);
  area->designer_table_name = table_name ? strdup(table_name) : NULL;

  area->designer_db_type = info->type;

  // Switch to Page 1 (Table Designer)
  TabCtrl_SetCurSel(area->hwndTab, 1);

  // Send TCN_SELCHANGE message manually to parent window to trigger visibility toggle!
  NMHDR nmhdr;
  nmhdr.hwndFrom = area->hwndTab;
  nmhdr.idFrom = IDC_TAB_CONTROL;
  nmhdr.code = TCN_SELCHANGE;
  SendMessageA(GetParent(area->hwndContainer), WM_NOTIFY, (WPARAM)IDC_TAB_CONTROL, (LPARAM)&nmhdr);
}
