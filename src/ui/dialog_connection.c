#include "dialog_connection.h"
#include "postgres.h"
#include "button.h"
#include <libpq-fe.h>
#define CLICKED "clicked"
#define PRESSED "pressed"
const char *DB_TYPE_NAMES[] = {"PostgreSQL", "MySQL", "SQLite"};

static void on_pg_button_press(GtkWidget *button, gpointer data){
    PgConnectContext *ctx = (PgConnectContext *)data;
    connect_postgresql(ctx);
    GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(button));
    gtk_window_destroy(parent);
}

static void on_db_selected(GtkWidget *button, gpointer data){
    DbSelectContext *context = (DbSelectContext *)data;
    DatabaseType db_type = context->db_type;
    Sidebar *sidebar = context->sidebar;
    switch (db_type){
    case POSTGRESQL:

        GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(button));
        gtk_window_destroy(GTK_WINDOW(gtk_widget_get_root(button)));
        GtkWidget *dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "PostgreSQL Connection");
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(button)));
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gtk_widget_set_margin_top(content, 15);
        gtk_widget_set_margin_bottom(content, 15);
        gtk_widget_set_margin_start(content, 15);
        gtk_widget_set_margin_end(content, 15);

        GtkWidget *err_host = gtk_label_new("");
        gtk_widget_set_visible(err_host, FALSE);
        GtkWidget *err_port = gtk_label_new("");
        gtk_widget_set_visible(err_port, FALSE);
        GtkWidget *err_user = gtk_label_new("");
        gtk_widget_set_visible(err_user, FALSE);
        GtkWidget *err_pass = gtk_label_new("");
        gtk_widget_set_visible(err_pass, FALSE);
        gtk_widget_add_css_class(err_host, "error");
        gtk_widget_add_css_class(err_port, "error");
        gtk_widget_add_css_class(err_user, "error");
        gtk_widget_add_css_class(err_pass, "error");

        GtkWidget *label = gtk_label_new("PostgreSQL Connection Details");
        GtkWidget *host = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(host), "Host");
        gtk_entry_set_activates_default(GTK_ENTRY(host), TRUE);
        GtkWidget *port = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(port), "Port");
        GtkWidget *username = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(username), "Username");
        GtkWidget *password = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(password), "Password");
        gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
        GtkWidget *db = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(db), "Database Name");

        GtkWidget *btn_connect = create_menu_button("Connect", NULL);
        PgForm *form = g_malloc(sizeof(PgForm));
        form->host = host;
        form->port = port;
        form->username = username;
        form->password = password;
        form->db = db;
        form->err_host = err_host;
        form->err_port = err_port;
        form->err_username = err_user;
        form->err_password = err_pass;

        GtkWidget *widgets[] = {
            label,
            host,err_host,
            port,err_port,
            username,err_user,
            password,err_pass,
            db,
            btn_connect
        };
        for (int i = 0; i < G_N_ELEMENTS(widgets); i++){
            gtk_box_append(GTK_BOX(content), widgets[i]);
        };
        PgConnectContext *ctx = g_malloc(sizeof(PgConnectContext));
        ctx->form = form;
        ctx->sidebar = sidebar;
        g_signal_connect(btn_connect, CLICKED, G_CALLBACK(on_pg_button_press), ctx);

        gtk_window_set_child(GTK_WINDOW(dialog), content);
        gtk_window_present(GTK_WINDOW(dialog));
        break;
    case MYSQL:
        g_print("MySQL selected\n");
        break;
    case SQLITE:
        g_print("SQLite selected\n");
        break;
    default:
        break;
    }
}

void show_create_connection_dialog(GtkWidget *widget, gpointer data){
    Sidebar *sidebar = (Sidebar *)data;
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "New Connection");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(widget)));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(content, 15);
    gtk_widget_set_margin_bottom(content, 15);
    gtk_widget_set_margin_start(content, 15);
    gtk_widget_set_margin_end(content, 15);

    GtkWidget *buttons[] = {
        create_menu_button(DB_TYPE_NAMES[POSTGRESQL], NULL),
        create_menu_button(DB_TYPE_NAMES[MYSQL], NULL),
        create_menu_button(DB_TYPE_NAMES[SQLITE], NULL),
        create_menu_button("Cancel", NULL)
    };

    DatabaseType db_types[] = {POSTGRESQL, MYSQL, SQLITE};

    for (int i = 0; i < 3; i++) {
        DbSelectContext *ctx = g_malloc(sizeof(DbSelectContext));
        ctx->sidebar = sidebar;
        ctx->db_type = db_types[i];
        g_signal_connect(buttons[i], CLICKED, G_CALLBACK(on_db_selected), ctx);
        gtk_box_append(GTK_BOX(content), buttons[i]);
    }

    g_signal_connect_swapped(buttons[3], CLICKED, G_CALLBACK(gtk_window_destroy), dialog);
    gtk_box_append(GTK_BOX(content), buttons[3]);

    gtk_window_set_child(GTK_WINDOW(dialog), content);
    gtk_window_present(GTK_WINDOW(dialog));
}
