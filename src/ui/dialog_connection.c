#include "dialog_connection.h"
#include "button.h"
#include <libpq-fe.h>
#define CLICKED "clicked"
#define PRESSED "pressed"
const char *DB_TYPE_NAMES[] = {"PostgreSQL", "MySQL", "SQLite"};

static void on_pg_button_press(GtkWidget *button, gpointer data)
{
    GtkWidget **credentials = (GtkWidget **)data;
    const char *host = gtk_editable_get_text(GTK_EDITABLE(credentials[0]));
    const char *port = gtk_editable_get_text(GTK_EDITABLE(credentials[1]));
    const char *username = gtk_editable_get_text(GTK_EDITABLE(credentials[2]));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(credentials[3]));
    const char *db_name = gtk_editable_get_text(GTK_EDITABLE(credentials[4]));
    /**
     * docs https://www.postgresql.org/docs/current/libpq.htmls ^^
     */
    char conninfo[512];
    if(strlen(db_name) == 0){
        snprintf(conninfo, sizeof(conninfo), "host=%s port=%s user=%s password=%s dbname=postgres",
                host, port, username, password);
    }else{
        snprintf(conninfo, sizeof(conninfo), "host=%s port=%s user=%s password=%s dbname=%s",
                host, port, username, password, db_name);
    }
    PGconn *conn = PQconnectdb(conninfo);
    if(PQstatus(conn) != CONNECTION_OK){
        g_print("failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
    } else {
      PGresult *res;
        if(strlen(db_name) == 0){
            res = PQexec(conn,
                "SELECT datname FROM pg_database WHERE datistemplate = false;"
            );
        } else {
            res = PQexec(conn, "SELECT current_database();");
        }

        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            int rows = PQntuples(res);
            for (int i = 0; i < rows; i++) {
                printf("Database: %s\n", PQgetvalue(res, i, 0));
            }
        }
        PQclear(res);
    }

    // g_free(conninfo);
    // g_free(credentials);
}

static void on_db_selected(GtkWidget *button, gpointer data)
{
    DatabaseType db_type = GPOINTER_TO_INT(data);
    switch (db_type)
    {
    case POSTGRESQL:
        gtk_window_destroy(GTK_WINDOW(gtk_widget_get_root(button)));
        GtkWidget *dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "PostgreSQL Connection");
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(button)));
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
        GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gtk_widget_set_margin_top(content, 15);
        gtk_widget_set_margin_bottom(content, 15);
        gtk_widget_set_margin_start(content, 15);
        gtk_widget_set_margin_end(content, 15);

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

        /**
         * @NOTED : is array of pointer every credential pointer 
         * and i allocate to heap 5 pointer gtkWidget , bisi poho pram lol
         */
        GtkWidget **credential = g_malloc(sizeof(GtkWidget *) * 5);
        credential[0] = host;
        credential[1] = port;
        credential[2] = username;
        credential[3] = password;
        credential[4] = db;

        gtk_box_append(GTK_BOX(content), label);
        gtk_box_append(GTK_BOX(content), host);
        gtk_box_append(GTK_BOX(content), port);
        gtk_box_append(GTK_BOX(content), username);
        gtk_box_append(GTK_BOX(content), password);
        gtk_box_append(GTK_BOX(content), db);
        gtk_box_append(GTK_BOX(content), btn_connect);

        g_signal_connect(btn_connect, CLICKED, G_CALLBACK(on_pg_button_press), credential);

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

void show_create_connection_dialog(GtkWidget *parent)
{
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "New Connection");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_root(parent)));
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

    g_signal_connect(btn_pg, CLICKED, G_CALLBACK(on_db_selected), GINT_TO_POINTER(POSTGRESQL));
    g_signal_connect(btn_mysql, CLICKED, G_CALLBACK(on_db_selected), GINT_TO_POINTER(MYSQL));
    g_signal_connect(btn_sqlite, CLICKED, G_CALLBACK(on_db_selected), GINT_TO_POINTER(SQLITE));
    g_signal_connect_swapped(btn_cancel, CLICKED, G_CALLBACK(gtk_window_destroy), dialog);

    gtk_box_append(GTK_BOX(content), btn_pg);
    gtk_box_append(GTK_BOX(content), btn_mysql);
    gtk_box_append(GTK_BOX(content), btn_sqlite);
    gtk_box_append(GTK_BOX(content), btn_cancel);

    gtk_window_set_child(GTK_WINDOW(dialog), content);
    gtk_window_present(GTK_WINDOW(dialog));
}
