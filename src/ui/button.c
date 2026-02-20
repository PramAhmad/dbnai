#include "button.h"

GtkWidget* create_menu_button(const char *title, GMenuModel *menu) {
    GtkWidget *button = gtk_menu_button_new();
    gtk_menu_button_set_label(GTK_MENU_BUTTON(button), title);
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(button), menu);
    gtk_widget_add_css_class(button, "nav-button");
    return button;
}
