#include "menu.h"

HMENU create_app_menu(void) {
  HMENU hMenuBar = CreateMenu();
  if (!hMenuBar)
    return NULL;

  // File Menu
  HMENU hFileMenu = CreatePopupMenu();
  AppendMenuA(hFileMenu, MF_STRING, IDM_FILE_NEW, "New");
  AppendMenuA(hFileMenu, MF_STRING, IDM_FILE_OPEN, "Open");
  AppendMenuA(hFileMenu, MF_STRING, IDM_FILE_SAVE, "Save");
  AppendMenuA(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, "Save As");
  AppendMenuA(hFileMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(hFileMenu, MF_STRING, IDM_FILE_EXIT, "Quit");
  AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");

  // Database Menu
  HMENU hDbMenu = CreatePopupMenu();
  AppendMenuA(hDbMenu, MF_STRING, IDM_DB_CONNECT, "New Connection");
  AppendMenuA(hDbMenu, MF_STRING, IDM_DB_DISCONNECT, "Disconnect");
  AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hDbMenu, "Database");

  // Help Menu
  HMENU hHelpMenu = CreatePopupMenu();
  AppendMenuA(hHelpMenu, MF_STRING, IDM_HELP_DOCS, "Documentation");
  AppendMenuA(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, "About");
  AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");

  return hMenuBar;
}
