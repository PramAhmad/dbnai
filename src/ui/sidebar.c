#include "sidebar.h"

GtkWidget* create_sidebar(void) {
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_add_css_class(sidebar, "sidebar");
    gtk_widget_set_size_request(sidebar, 200, -1);

    GtkWidget *sidebar_label = gtk_label_new("Sidebar");
    gtk_box_append(GTK_BOX(sidebar), sidebar_label);

    return sidebar;
}
