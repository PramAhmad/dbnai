#include <gtk/gtk.h>
#include "ui/navbar.h"
#include "ui/sidebar.h"
#include "ui/content.h"
#include "style/css.h"

#define TITLE "DBNAI"


static void on_app_new(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: New\n");
}

static void on_app_open(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: Open\n");
}

static void on_app_save(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: Save\n");
}

static void on_app_save_as(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: Save As\n");
}

static void on_app_quit(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    GApplication *app = G_APPLICATION(user_data);
    g_application_quit(app);
}

static void on_app_connect(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: Connect\n");
}

static void on_app_disconnect(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: Disconnect\n");
}

static void on_app_documentation(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: Documentation\n");
}

static void on_app_about(GSimpleAction *action, GVariant *param, gpointer user_data)
{
    g_print("App: About\n");
}

static void activate(GtkApplication *app, gpointer user_data)
{
    setup_css();

    const GActionEntry app_entries[] = {
        { "new", on_app_new, NULL, NULL, NULL },
        { "open", on_app_open, NULL, NULL, NULL },
        { "save", on_app_save, NULL, NULL, NULL },
        { "save_as", on_app_save_as, NULL, NULL, NULL },
        { "quit", on_app_quit, NULL, NULL, NULL },
        { "connect", on_app_connect, NULL, NULL, NULL },
        { "disconnect", on_app_disconnect, NULL, NULL, NULL },
        { "documentation", on_app_documentation, NULL, NULL, NULL },
        { "about", on_app_about, NULL, NULL, NULL }
    };

    g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);

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
