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

  return area;
}

void resize_content_area(ContentArea *area, int x, int y, int width,
                         int height) {
  if (!area)
    return;

  MoveWindow(area->hwndContainer, x, y, width, height, TRUE);
  MoveWindow(area->hwndTitle, 5, 5, width - 10, 20, TRUE);
  MoveWindow(area->hwndRun, 5, 30, 80, 25, TRUE);

  int editor_height = (height - 250) / 2;
  if (editor_height < 80)
    editor_height = 80;

  MoveWindow(area->hwndEditor, 5, 60, width - 10, editor_height, TRUE);
  MoveWindow(area->hwndStatus, 5, 60 + editor_height + 5, width - 10, 20, TRUE);

  int grid_y = 60 + editor_height + 30;
  int grid_height = height - grid_y - 5;
  if (grid_height < 50)
    grid_height = 50;

  MoveWindow(area->hwndResultGrid, 5, grid_y, width - 10, grid_height, TRUE);
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

  free(query);
}
