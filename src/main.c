#include <gtk/gtk.h>
#include "ui/navbar.h"
#include "ui/sidebar.h"
#include "ui/content.h"

#define TITLE "DBNAI"

static void setup_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        ".navbar {"
        "  border-bottom: 1px solid #ddd;"
        "  padding: 0 6px;"
        "}"
        "button {"
        "  border: none;"
        "  background: transparent;"
        "  padding: 0;"
        "  margin: 0;"
        "  box-shadow: none;"
        "  font-weight: normal;"
        "}"
        "button:hover {"
        "  background: #eaeaea;"
        "  border-radius: 4px;"
        "}"
        ".sidebar {"
        "  background: #f5f5f5;"
        "  border-right: 1px solid #ddd;"
        "  padding: 10px;"
        "}"
        ".content-area {"
        "  padding: 10px;"
        "}",
        -1);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data)
{
    setup_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), TITLE);
    gtk_window_maximize(GTK_WINDOW(window));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(vbox, -5);
    gtk_widget_set_margin_bottom(vbox, 5);

    GtkWidget *navbar = create_navbar(app);

    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(main_hbox, TRUE);
    gtk_widget_set_hexpand(main_hbox, TRUE);

    GtkWidget *sidebar = create_sidebar();
    GtkWidget *content = create_content_area();

    gtk_box_append(GTK_BOX(main_hbox), sidebar);
    gtk_box_append(GTK_BOX(main_hbox), content);

    gtk_box_append(GTK_BOX(vbox), navbar);
    gtk_box_append(GTK_BOX(vbox), main_hbox);

    gtk_window_set_child(GTK_WINDOW(window), vbox);
    gtk_widget_show(window);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("com.example.navbar", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
