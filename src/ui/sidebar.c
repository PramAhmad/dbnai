#include "sidebar.h"

static void on_context_new(GSimpleAction *action, GVariant *param, gpointer data)
{
    g_print("Create coi\n");
}

static void on_context_edit(GSimpleAction *action, GVariant *param, gpointer data)
{
    g_print("Edit Coii\n");
}

static void on_context_delete(GSimpleAction *action, GVariant *param, gpointer data)
{
    g_print("Delete coii\n");
}

static void on_context_new_clicked(GtkWidget *button, gpointer data)
{
    g_print("Create coi\n");
}

static void on_context_edit_clicked(GtkWidget *button, gpointer data)
{
    g_print("Edit Coii\n");
}

static void on_db_selected(GtkWidget *button, gpointer data)
{
    const char *db_type = gtk_button_get_label(GTK_BUTTON(button));
    g_print("Selected DB: %s\n", db_type);
}

static void show_create_connection_dialog(GtkWidget *parent)
{
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Create Connection");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(parent)));

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(content, 15);
    gtk_widget_set_margin_bottom(content, 15);
    gtk_widget_set_margin_start(content, 15);
    gtk_widget_set_margin_end(content, 15);

    GtkWidget *btn_pg = gtk_button_new_with_label("PostgreSQL");
    GtkWidget *btn_mysql = gtk_button_new_with_label("MySQL");
    GtkWidget *btn_sqlite = gtk_button_new_with_label("SQLite");
    GtkWidget *btn_cancel = gtk_button_new_with_label("Cancel");

    g_signal_connect(btn_pg, "clicked", G_CALLBACK(on_db_selected), NULL);
    g_signal_connect(btn_mysql, "clicked", G_CALLBACK(on_db_selected), NULL);
    g_signal_connect(btn_sqlite, "clicked", G_CALLBACK(on_db_selected), NULL);
    g_signal_connect_swapped(btn_cancel, "clicked", G_CALLBACK(gtk_window_destroy), dialog);

    gtk_box_append(GTK_BOX(content), btn_pg);
    gtk_box_append(GTK_BOX(content), btn_mysql);
    gtk_box_append(GTK_BOX(content), btn_sqlite);
    gtk_box_append(GTK_BOX(content), btn_cancel);

    gtk_window_set_child(GTK_WINDOW(dialog), content);
    gtk_window_present(GTK_WINDOW(dialog));
}

static GtkPopover *create_context_menu(GtkWidget *item)
{
    GtkPopover *popover = GTK_POPOVER(gtk_popover_new());
    gtk_popover_set_autohide(popover, TRUE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);

    GtkWidget *btn_new = gtk_button_new_with_label("New Connection");
    GtkWidget *btn_edit = gtk_button_new_with_label("Edit Connection");

    g_signal_connect(btn_new, "clicked", G_CALLBACK(on_context_new_clicked), item);
    g_signal_connect(btn_edit, "clicked", G_CALLBACK(on_context_edit_clicked), item);

    gtk_box_append(GTK_BOX(box), btn_new);
    gtk_box_append(GTK_BOX(box), btn_edit);

    gtk_popover_set_child(popover, box);

    return popover;
}

static gboolean on_right_click(GtkGestureClick *gesture,
                               int n_press,
                               double x,
                               double y,
                               gpointer user_data)
{
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    GtkPopover *popover = GTK_POPOVER(user_data);

    GdkRectangle rect = {(int)x, (int)y, 1, 1};
    gtk_popover_set_pointing_to(popover, &rect);
    gtk_popover_popup(popover);
    return TRUE;
}

GtkWidget *create_sidebar(void)
{
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(sidebar, 200, -1);
    gtk_widget_add_css_class(sidebar, "sidebar");

    GtkWidget *btn_create = gtk_button_new_with_label("Create Connection");

    gtk_box_append(GTK_BOX(sidebar), btn_create);

    g_signal_connect(btn_create, "clicked",
                     G_CALLBACK(show_create_connection_dialog),
                     sidebar);

    GtkPopover *popover = create_context_menu(sidebar);
    gtk_widget_set_parent(GTK_WIDGET(popover), sidebar);

    GtkGestureClick *gesture = GTK_GESTURE_CLICK(gtk_gesture_click_new());
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_right_click), popover);
    gtk_widget_add_controller(sidebar, GTK_EVENT_CONTROLLER(gesture));

    return sidebar;
}