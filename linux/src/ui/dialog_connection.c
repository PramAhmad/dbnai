#include "dialog_connection.h"
#include "postgres.h"
#include "mysql_handler.h"
#include "button.h"
#include <libpq-fe.h>
#define CLICKED "clicked"
#define PRESSED "pressed"
const char *DB_TYPE_NAMES[] = {"PostgreSQL", "MySQL", "SQLite"};

static void on_connection_button_press(GtkWidget *button, gpointer data){
    SqlConnectContext *ctx = (SqlConnectContext *)data;
    
    switch (ctx->db_type) {
        case POSTGRESQL:
            connect_postgresql(ctx);
            break;
        case DB_MYSQL:
            connect_mysql(ctx);
            break;
        case SQLITE:
            g_print("SQLite not implemented yet\n");
            break;
        default:
            break;
    }
    
    GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(button));
    gtk_window_destroy(parent);
}

static GtkWidget* window_form_pg(SqlForm *form, GtkWidget **btn_connect_out){
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "PostgreSQL Connection");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(dialog)));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(content, 15);
    gtk_widget_set_margin_bottom(content, 15);
    gtk_widget_set_margin_start(content, 15);
    gtk_widget_set_margin_end(content, 15);

    GtkWidget *label = gtk_label_new("PostgreSQL Connection Details");
    GtkWidget *conn_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(conn_name), "Connection Name");
    GtkWidget *host = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(host), "Host");
    GtkWidget *port = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(port), "Port (default: 5432)");
    GtkWidget *username = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username), "Username");
    GtkWidget *password = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
    GtkWidget *db = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(db), "Database Name (optional)");

    form->name = conn_name;
    form->host = host;
    form->port = port;
    form->username = username;
    form->password = password;
    form->db = db;

    GtkWidget *btn_connect = create_menu_button("Connect", NULL);

    GtkWidget *widgets[] = {
        label,
        conn_name,
        host,
        port,
        username,
        password,
        db,
        btn_connect
    };
    for (int i = 0; i < G_N_ELEMENTS(widgets); i++){
        gtk_box_append(GTK_BOX(content), widgets[i]);
    };
    
    *btn_connect_out = btn_connect;
    gtk_window_set_child(GTK_WINDOW(dialog), content);
    return dialog;
}

static GtkWidget* window_form_mysql(SqlForm *form, GtkWidget **btn_connect_out){
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "MySQL Connection");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(dialog)));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(content, 15);
    gtk_widget_set_margin_bottom(content, 15);
    gtk_widget_set_margin_start(content, 15);
    gtk_widget_set_margin_end(content, 15);

    GtkWidget *label = gtk_label_new("MySQL Connection Details");
    GtkWidget *conn_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(conn_name), "Connection Name");
    GtkWidget *host = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(host), "Host");
    GtkWidget *port = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(port), "Port (default: 3306)");
    GtkWidget *username = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username), "Username");
    GtkWidget *password = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
    GtkWidget *db = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(db), "Database Name (optional)");

    form->name = conn_name;
    form->host = host;
    form->port = port;
    form->username = username;
    form->password = password;
    form->db = db;

    GtkWidget *btn_connect = create_menu_button("Connect", NULL);

    GtkWidget *widgets[] = {
        label,
        conn_name,
        host,
        port,
        username,
        password,
        db,
        btn_connect
    };
    for (int i = 0; i < G_N_ELEMENTS(widgets); i++){
        gtk_box_append(GTK_BOX(content), widgets[i]);
    };
    
    *btn_connect_out = btn_connect;
    gtk_window_set_child(GTK_WINDOW(dialog), content);
    return dialog;
}

static void on_db_selected(GtkWidget *button, gpointer data){
    DbSelectContext *context = (DbSelectContext *)data;
    DatabaseType db_type = context->db_type;
    Sidebar *sidebar = context->sidebar;
    
    GtkWidget *btn_connect = NULL;
    GtkWidget *dialog = NULL;
    SqlForm *form = g_malloc(sizeof(SqlForm));
    SqlConnectContext *ctx = NULL;
    
    switch (db_type){
        case POSTGRESQL:
            dialog = window_form_pg(form, &btn_connect);
            ctx = g_malloc(sizeof(SqlConnectContext));
            ctx->form = form;
            ctx->sidebar = sidebar;
            ctx->conn_store = sidebar->conn_store;
            ctx->db_type = POSTGRESQL;
            g_signal_connect(btn_connect, CLICKED, G_CALLBACK(on_connection_button_press), ctx);
            gtk_window_present(GTK_WINDOW(dialog));
            break;
            
        case DB_MYSQL:
            dialog = window_form_mysql(form, &btn_connect);
            ctx = g_malloc(sizeof(SqlConnectContext));
            ctx->form = form;
            ctx->sidebar = sidebar;
            ctx->conn_store = sidebar->conn_store;
            ctx->db_type = DB_MYSQL;
            g_signal_connect(btn_connect, CLICKED, G_CALLBACK(on_connection_button_press), ctx);
            gtk_window_present(GTK_WINDOW(dialog));
            break;
            
        case SQLITE:
            g_print("SQLite not implemented yet\n");
            g_free(form);
            break;
            
        default:
            g_free(form);
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
        create_menu_button(DB_TYPE_NAMES[DB_MYSQL], NULL),
        create_menu_button(DB_TYPE_NAMES[SQLITE], NULL),
        create_menu_button("Cancel", NULL)
    };

    DatabaseType db_types[] = {POSTGRESQL, DB_MYSQL, SQLITE};

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
