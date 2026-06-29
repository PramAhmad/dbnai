#ifndef UI_MENU_H
#define UI_MENU_H

#include <windows.h>

#define IDM_FILE_NEW 1001
#define IDM_FILE_OPEN 1002
#define IDM_FILE_SAVE 1003
#define IDM_FILE_SAVEAS 1004
#define IDM_FILE_EXIT 1005

#define IDM_DB_CONNECT 2001
#define IDM_DB_DISCONNECT 2002

#define IDM_HELP_DOCS 3001
#define IDM_HELP_ABOUT 3002

HMENU create_app_menu(void);

#endif
