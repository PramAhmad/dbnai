#ifndef UI_CONTENT_H
#define UI_CONTENT_H

#include "sidebar.h"
#include <commctrl.h>
#include <windows.h>

#define IDC_BTN_RUN 5001    // Run SQL query button
#define IDC_EDT_SQL 5002    // SQL query editor
#define IDC_LST_RESULT 5003 // Result grid
#define IDC_TAB_CONTROL 5004 // Tab control
#define IDC_LST_DESIGNER 5005 // Designer columns list
#define IDC_EDT_ADD_NAME 5006 // Add column name edit
#define IDC_EDT_ADD_TYPE 5007 // Add column type edit
#define IDC_BTN_ADD_COL 5008  // Add column button
#define IDC_BTN_DROP_COL 5009 // Drop column button

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

  HWND hwndTab;
  HWND hwndDesignerTitle;
  HWND hwndDesignerGrid;
  HWND hwndAddName;
  HWND hwndAddType;
  HWND hwndBtnAdd;
  HWND hwndBtnDrop;

  char *designer_conn_id;
  char *designer_db_name;
  char *designer_schema_name;
  char *designer_table_name;
  DbType designer_db_type;
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
void content_area_set_query(ContentArea *area, const char *conn_id,
                            const char *db_name, const char *query,
                            BOOL execute);
void content_area_show_table_designer(ContentArea *area, const char *conn_id,
                                      const char *db_name, const char *schema_name,
                                      const char *table_name, ConnectionInfo *info);

#endif
