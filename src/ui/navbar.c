#include "navbar.h"
#include "button.h"

GtkWidget* create_navbar(GtkApplication *app) {
    GtkWidget *navbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_add_css_class(navbar, "navbar");
    gtk_widget_set_size_request(navbar, -1, 40);

    MenuItem files_items[] = {
        {"New", "app.new"},
        {"Open", "app.open"},
        {"Save", "app.save"},
        {"Save As", "app.save_as"},
        {"Quit", "app.quit"}
    };

    GMenu *file_menu = g_menu_new();
    for (size_t i = 0; i < sizeof(files_items) / sizeof(MenuItem); i++) {
        g_menu_append(file_menu, files_items[i].label, files_items[i].action);
    }

    MenuItem database_items[] = {
        {"Connect", "app.connect"},
        {"Disconnect", "app.disconnect"}
    };
    GMenu *database_menu = g_menu_new();
    for (size_t i = 0; i < sizeof(database_items) / sizeof(MenuItem); i++) {
        g_menu_append(database_menu, database_items[i].label, database_items[i].action);
    }

    /* Help Menu */
    MenuItem help_items[] = {
        {"Documentation", "app.documentation"},
        {"About", "app.about"}
    };
    GMenu *help_menu = g_menu_new();
    for (size_t i = 0; i < sizeof(help_items) / sizeof(MenuItem); i++) {
        g_menu_append(help_menu, help_items[i].label, help_items[i].action);
    }

    GtkWidget *file_button = create_menu_button("File", G_MENU_MODEL(file_menu));
    GtkWidget *database_button = create_menu_button("Database", G_MENU_MODEL(database_menu));
    GtkWidget *help_button = create_menu_button("Help", G_MENU_MODEL(help_menu));

    gtk_box_append(GTK_BOX(navbar), file_button);
    gtk_box_append(GTK_BOX(navbar), database_button);
    gtk_box_append(GTK_BOX(navbar), help_button);

    /**
     * clean cause now the menu is owned by buttons+-+ bisi memory leaks
     */
    g_object_unref(file_menu);
    g_object_unref(database_menu);
    g_object_unref(help_menu);

    return navbar;
}
