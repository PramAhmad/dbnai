#include "dialog_connection.h"
#include "button.h"
#include <libpq-fe.h>
#define CLICKED "clicked"
#define PRESSED "pressed"
const char *DB_TYPE_NAMES[] = {"PostgreSQL", "MySQL", "SQLite"};

static void on_pg_button_press(GtkWidget *button, gpointer data){
    PgConnectContext *ctx = (PgConnectContext *)data;
    PgForm *form = ctx->form;
    const char *host = gtk_editable_get_text(GTK_EDITABLE(form->host));
    const char *port = gtk_editable_get_text(GTK_EDITABLE(form->port));
    const char *username = gtk_editable_get_text(GTK_EDITABLE(form->username));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(form->password));
    const char *db_name = gtk_editable_get_text(GTK_EDITABLE(form->db));

    gboolean valid = TRUE;

    if (strlen(host) == 0){
        gtk_label_set_text(GTK_LABEL(form->err_host), "Host wajib diisi");
        gtk_widget_set_visible(form->err_host, TRUE);
        valid = FALSE;
    }else{
        gtk_widget_set_visible(form->err_host, FALSE);
    }

    if (strlen(port) == 0){
        gtk_label_set_text(GTK_LABEL(form->err_port), "Port wajib diisi");
        gtk_widget_set_visible(form->err_port, TRUE);
        valid = FALSE;
    }else{
        gtk_widget_set_visible(form->err_port, FALSE);
    }

    if (strlen(username) == 0){
        gtk_label_set_text(GTK_LABEL(form->err_username), "Username wajib diisi");
        gtk_widget_set_visible(form->err_username, TRUE);
        valid = FALSE;
    }else{
        gtk_widget_set_visible(form->err_username, FALSE);
    }

    if (strlen(password) == 0){
        gtk_label_set_text(GTK_LABEL(form->err_password), "Password wajib diisi");
        gtk_widget_set_visible(form->err_password, TRUE);
        valid = FALSE;
    }else{
        gtk_widget_set_visible(form->err_password, FALSE);
    }

    if (!valid) return;

  
    /**
     * docs https://www.postgresql.org/docs/current/libpq.htmls ^^
     */
    char conninfo[512];
    if (strlen(db_name) == 0){
        snprintf(conninfo, sizeof(conninfo), "host=%s port=%s user=%s password=%s dbname=postgres",
                 host, port, username, password);
    }else{
        snprintf(conninfo, sizeof(conninfo), "host=%s port=%s user=%s password=%s dbname=%s",
                 host, port, username, password, db_name);
    }
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK){
        g_print("failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
    }else{
        PGresult *res;
        if (strlen(db_name) == 0){
            res = PQexec(conn, "SELECT datname FROM pg_database WHERE datistemplate = false;");
        }else{
            res = PQexec(conn, "SELECT current_database();");
        }

      if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        gtk_tree_store_clear(ctx->sidebar->store);
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            const char *db = PQgetvalue(res, i, 0);
            GtkTreeIter db_iter;
            gtk_tree_store_append(ctx->sidebar->store, &db_iter, NULL);
            gtk_tree_store_set(ctx->sidebar->store, &db_iter, 0, db, -1);
            // conn ke db nya buat ambil skema
            char conninfo_db[512];
            snprintf(conninfo_db, sizeof(conninfo_db),
                 "host=%s port=%s user=%s password=%s dbname=%s",
                 host, port, username, password, db);

            PGconn *db_conn = PQconnectdb(conninfo_db);
            if (PQstatus(db_conn) != CONNECTION_OK) {
                PQfinish(db_conn);
                continue;
            }

            PGresult *schemas = PQexec(db_conn,
                "SELECT schema_name FROM information_schema.schemata "
                "WHERE schema_name NOT LIKE 'pg_%' "
                "AND schema_name != 'information_schema';");

            for (int s = 0; s < PQntuples(schemas); s++) {
                const char *schema_name = PQgetvalue(schemas, s, 0);

                GtkTreeIter schema_iter;
                gtk_tree_store_append(ctx->sidebar->store, &schema_iter, &db_iter);
                gtk_tree_store_set(ctx->sidebar->store, &schema_iter, 0, schema_name, -1);
                // scrollable tree store
                char query[256];
                snprintf(query, sizeof(query),
                        "SELECT table_name FROM information_schema.tables "
                        "WHERE table_schema='%s';",
                        schema_name);

                PGresult *tables = PQexec(db_conn, query);

                for (int t = 0; t < PQntuples(tables); t++) {
                    GtkTreeIter table_iter;
                    gtk_tree_store_append(ctx->sidebar->store, &table_iter, &schema_iter);
                    gtk_tree_store_set(ctx->sidebar->store, &table_iter, 0,
                                    PQgetvalue(tables, t, 0), -1);
                }

                PQclear(tables);
            }

        PQclear(schemas);
        PQfinish(db_conn);
    }
}
        PQclear(res);
    }

    // g_free(conninfo);
    // g_free(credentials);
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

    GtkWidget *btn_pg = create_menu_button(DB_TYPE_NAMES[POSTGRESQL], NULL);
    GtkWidget *btn_mysql = create_menu_button(DB_TYPE_NAMES[MYSQL], NULL);
    GtkWidget *btn_sqlite = create_menu_button(DB_TYPE_NAMES[SQLITE], NULL);
    GtkWidget *btn_cancel = create_menu_button("Cancel", NULL);

    DbSelectContext *pg_ctx = g_malloc(sizeof(DbSelectContext));
    pg_ctx->sidebar = sidebar;
    pg_ctx->db_type = POSTGRESQL;
    g_signal_connect(btn_pg, CLICKED, G_CALLBACK(on_db_selected), pg_ctx);
    DbSelectContext *mysql_ctx = g_malloc(sizeof(DbSelectContext));
    mysql_ctx->sidebar = sidebar;
    mysql_ctx->db_type = MYSQL;
    g_signal_connect(btn_mysql, CLICKED, G_CALLBACK(on_db_selected), mysql_ctx);
    DbSelectContext *sqlite_ctx = g_malloc(sizeof(DbSelectContext));
    sqlite_ctx->sidebar = sidebar;
    sqlite_ctx->db_type = SQLITE;
    g_signal_connect(btn_sqlite, CLICKED, G_CALLBACK(on_db_selected), sqlite_ctx);
    g_signal_connect_swapped(btn_cancel, CLICKED, G_CALLBACK(gtk_window_destroy), dialog);

    gtk_box_append(GTK_BOX(content), btn_pg);
    gtk_box_append(GTK_BOX(content), btn_mysql);
    gtk_box_append(GTK_BOX(content), btn_sqlite);
    gtk_box_append(GTK_BOX(content), btn_cancel);

    gtk_window_set_child(GTK_WINDOW(dialog), content);
    gtk_window_present(GTK_WINDOW(dialog));
}
