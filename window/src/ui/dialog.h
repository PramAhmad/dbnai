#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "sidebar.h"
#include <windows.h>

void show_new_connection_dialog(HWND hwndParent, HINSTANCE hInst,
                                Sidebar *sidebar);

#endif
