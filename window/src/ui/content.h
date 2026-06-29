#ifndef UI_CONTENT_H
#define UI_CONTENT_H

#include "sidebar.h"
#include <commctrl.h>
#include <windows.h>

#define IDC_BTN_RUN 5001    // Run SQL query button
#define IDC_EDT_SQL 5002    // SQL query editor
#define IDC_LST_RESULT 5003 // Result grid

typedef struct {
  HWND hwndContainer;
  HWND hwndTitle;
  HWND hwndEditor;
  HWND hwndRun;
  HWND hwndStatus;
  HWND hwndResultGrid;
  Sidebar *sidebar;
  char *active_conn_id;
  char *active_db;
} ContentArea;

ContentArea *create_content_area(HWND hwndParent, HINSTANCE hInst);
void resize_content_area(ContentArea *area, int x, int y, int width,
                         int height);
void content_attach_sidebar(ContentArea *area, Sidebar *sidebar);
void set_status_text(ContentArea *area, const char *text);
void clear_result_grid(ContentArea *area);
void run_sql_query(ContentArea *area);
void apply_connection_context(ContentArea *area, const char *conn_id,
                              const char *db_name);

#endif
