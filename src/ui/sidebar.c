#include "sidebar.h"
#include "button.h"
#include "dialog_connection.h"
#define CLICKED "clicked"
#define PRESSED "pressed"


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

static void on_context_edit_clicked(GtkWidget *button, gpointer data)
{
    g_print("Edit Coii\n");
}



static GtkPopover *create_context_menu(GtkWidget *item)
{
    GtkPopover *popover = GTK_POPOVER(gtk_popover_new());
    gtk_popover_set_autohide(popover, TRUE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);

    GtkWidget *btn_new = create_menu_button("New Connection", NULL);
    GtkWidget *btn_edit = create_menu_button("Edit Connection", NULL);
    gtk_widget_add_css_class(btn_new, "buttontransparent");
    gtk_widget_add_css_class(btn_edit, "buttontransparent");

    g_signal_connect(btn_new, CLICKED, G_CALLBACK(show_create_connection_dialog), item);
    g_signal_connect(btn_edit, CLICKED, G_CALLBACK(on_context_edit_clicked), item);

    gtk_box_append(GTK_BOX(box), btn_new);
    gtk_box_append(GTK_BOX(box), btn_edit);

    gtk_popover_set_child(popover, box);

    return popover;
}

static gboolean on_right_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
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
    Sidebar *sidebar = g_malloc(sizeof(Sidebar));

    sidebar->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(sidebar->container, 200, -1);
    gtk_widget_add_css_class(sidebar->container, "sidebar");

    GtkWidget *btn_create = create_menu_button("Create Connection", NULL);
    gtk_box_append(GTK_BOX(sidebar->container), btn_create);

    sidebar->store = gtk_tree_store_new(1, G_TYPE_STRING);
    sidebar->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sidebar->store));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
    gtk_tree_view_column_new_with_attributes("Connections", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sidebar->treeview), column);

    gtk_box_append(GTK_BOX(sidebar->container), sidebar->treeview);

    // Attach sidebar struct to widget
    g_object_set_data_full(G_OBJECT(sidebar->container), "sidebar", sidebar, g_free);

    g_signal_connect(btn_create, CLICKED,
                     G_CALLBACK(show_create_connection_dialog),
                     sidebar);

    return sidebar->container;
}