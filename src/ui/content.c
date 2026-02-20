#include "content.h"

GtkWidget* create_content_area(void) {
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_add_css_class(content, "content-area");
    gtk_widget_set_hexpand(content, TRUE);
    gtk_widget_set_vexpand(content, TRUE);

    GtkWidget *content_label = gtk_label_new("Content Area");
    gtk_box_append(GTK_BOX(content), content_label);

    return content;
}
