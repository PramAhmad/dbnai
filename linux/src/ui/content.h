#ifndef UI_CONTENT_H
#define UI_CONTENT_H

#include <gtk/gtk.h>
#include "sidebar.h"

GtkWidget* create_content_area(void);
void content_attach_sidebar(GtkWidget *content, Sidebar *sidebar);

#endif
