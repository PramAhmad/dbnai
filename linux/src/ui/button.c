#include "button.h"

static void on_menu_button_clicked(GtkWidget *button, gpointer user_data)
{
    GtkPopover *popover = GTK_POPOVER(user_data);
    if (gtk_widget_get_visible(GTK_WIDGET(popover))) {
        gtk_popover_popdown(popover);
    } else {
        gtk_popover_popup(popover);
    }
}

GtkWidget* create_menu_button(const char *title, GMenuModel *menu) {
    GtkWidget *button = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(button), title);

    // case: if the menu is NULL, we just create a normal button without a popover
    if (menu) {
        GtkWidget *popover = gtk_popover_menu_new_from_model(menu);
        gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        gtk_widget_set_parent(popover, button);
        g_signal_connect(button, "clicked", G_CALLBACK(on_menu_button_clicked), popover);
    }

    return button;
}
