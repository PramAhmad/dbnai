#include "content.h"
#include "connection_store.h"
#include <libpq-fe.h>
#include <string.h>

typedef struct {
    GtkWidget *root;
    GtkWidget *title_label;
    GtkWidget *run_button;
    GtkWidget *editor;
    GtkWidget *status_label;
    GtkWidget *result_tree;
    Sidebar *sidebar;
    char *active_conn_id;
    char *active_db;
    GHashTable *query_by_conn;
} ContentArea;

static void clear_tree_columns(GtkTreeView *tree_view) {
    GList *columns = gtk_tree_view_get_columns(tree_view);
    for (GList *l = columns; l != NULL; l = l->next) {
        gtk_tree_view_remove_column(tree_view, GTK_TREE_VIEW_COLUMN(l->data));
    }
    g_list_free(columns);
}

static void set_status(ContentArea *area, const char *message) {
    gtk_label_set_text(GTK_LABEL(area->status_label), message ? message : "");
}

static char *build_connection_string(const ConnectionInfo *info, const char *db_name) {
    const char *database = db_name;
    if (!database || strlen(database) == 0) {
        database = (info->database && strlen(info->database) > 0) ? info->database : "postgres";
    }

    return g_strdup_printf(
        "host=%s port=%s user=%s password=%s dbname=%s",
        info->host,
        info->port,
        info->username,
        info->password,
        database
    );
}

static void render_query_result(ContentArea *area, PGresult *res) {
    GtkTreeView *tree_view = GTK_TREE_VIEW(area->result_tree);
    clear_tree_columns(tree_view);

    int cols = PQnfields(res);
    if (cols <= 0) {
        GtkListStore *empty_store = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(empty_store));
        g_object_unref(empty_store);
        return;
    }

    GType *types = g_new0(GType, cols);
    for (int i = 0; i < cols; i++) {
        types[i] = G_TYPE_STRING;
    }

    GtkListStore *store = gtk_list_store_newv(cols, types);
    g_free(types);

    for (int c = 0; c < cols; c++) {
        const char *name = PQfname(res, c);
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(name, renderer, "text", c, NULL);
        gtk_tree_view_append_column(tree_view, column);
    }

    int rows = PQntuples(res);
    for (int r = 0; r < rows; r++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        for (int c = 0; c < cols; c++) {
            const char *value = PQgetisnull(res, r, c) ? "NULL" : PQgetvalue(res, r, c);
            gtk_list_store_set(store, &iter, c, value, -1);
        }
    }

    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(store));
    g_object_unref(store);
}

static void run_sql_query(ContentArea *area) {
    if (!area->sidebar || !area->active_conn_id) {
        set_status(area, "Please select a PostgreSQL connection first.");
        return;
    }

    ConnectionInfo *conn_info = connection_store_get(area->sidebar->conn_store, area->active_conn_id);
    if (!conn_info) {
        set_status(area, "Not Found Metadata.");
        return;
    }

    if (conn_info->type != DB_TYPE_POSTGRESQL) {
        set_status(area, "SQL runner currently supports PostgreSQL only.");
        return;
    }


    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(area->editor));
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    char *query = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (!query || strlen(query) == 0) {
        set_status(area, "Query is empty.");
        g_free(query);
        return;
    }

    g_hash_table_replace(area->query_by_conn, g_strdup(area->active_conn_id), g_strdup(query));

    char *conn_str = build_connection_string(conn_info, area->active_db);
    PGconn *conn = PQconnectdb(conn_str);
    g_free(conn_str);

    if (PQstatus(conn) != CONNECTION_OK) {
        set_status(area, PQerrorMessage(conn));
        PQfinish(conn);
        g_free(query);
        return;
    }

    PGresult *res = PQexec(conn, query);
    ExecStatusType status = PQresultStatus(res);

    if (status == PGRES_TUPLES_OK) {
        render_query_result(area, res);
        char *msg = g_strdup_printf("OK: %d row(s)", PQntuples(res));
        set_status(area, msg);
        g_free(msg);
    } else if (status == PGRES_COMMAND_OK) {
        clear_tree_columns(GTK_TREE_VIEW(area->result_tree));
        gtk_tree_view_set_model(GTK_TREE_VIEW(area->result_tree), NULL);

        const char *tuples = PQcmdTuples(res);
        if (tuples && strlen(tuples) > 0) {
            char *msg = g_strdup_printf("OK: affected rows %s", tuples);
            set_status(area, msg);
            g_free(msg);
        } else {
            set_status(area, "OK: command executed.");
        }
    } else {
        set_status(area, PQerrorMessage(conn));
    }

    PQclear(res);
    PQfinish(conn);
    g_free(query);
}

static void on_run_clicked(GtkWidget *button, gpointer user_data) {
    ContentArea *area = (ContentArea *)user_data;
    run_sql_query(area);
}

static gboolean on_editor_key_pressed(GtkEventControllerKey *controller,
                                      guint keyval,
                                      guint keycode,
                                      GdkModifierType state,
                                      gpointer user_data) {
    ContentArea *area = (ContentArea *)user_data;
    gboolean ctrl_pressed = (state & GDK_CONTROL_MASK) != 0;

    if (ctrl_pressed && keyval == GDK_KEY_Return) {
        run_sql_query(area);
        return TRUE;
    }

    return FALSE;
}

static void apply_connection_context(ContentArea *area, const char *conn_id, const char *db_name) {
    g_free(area->active_conn_id);
    area->active_conn_id = g_strdup(conn_id);

    g_free(area->active_db);
    area->active_db = g_strdup(db_name);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(area->editor));
    const char *saved = conn_id ? g_hash_table_lookup(area->query_by_conn, conn_id) : NULL;
    gtk_text_buffer_set_text(buffer, saved ? saved : "", -1);

    if (!conn_id) {
        gtk_label_set_text(GTK_LABEL(area->title_label), "SQL Editor (PostgreSQL)");
        gtk_widget_set_sensitive(area->run_button, FALSE);
        set_status(area, "Select a connection/database from sidebar to start querying.");
        return;
    }

    ConnectionInfo *info = connection_store_get(area->sidebar->conn_store, conn_id);
    if (!info) {
        gtk_widget_set_sensitive(area->run_button, FALSE);
        set_status(area, "Connection metadata not found.");
        return;
    }

    if (info->type != DB_TYPE_POSTGRESQL) {
        gtk_widget_set_sensitive(area->run_button, FALSE);
        set_status(area, "SQL runner currently supports PostgreSQL only.");
        return;
    }

    gtk_widget_set_sensitive(area->run_button, TRUE);

    const char *display_db = (db_name && strlen(db_name) > 0) ? db_name : ((info->database && strlen(info->database) > 0) ? info->database : "postgres");
    char *title = g_strdup_printf("SQL Editor - %s / %s", info->name, display_db);
    gtk_label_set_text(GTK_LABEL(area->title_label), title);
    g_free(title);
    set_status(area, "Ready. Run with button or Ctrl+Enter.");
}

static void on_sidebar_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    ContentArea *area = (ContentArea *)user_data;

    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        apply_connection_context(area, NULL, NULL);
        return;
    }

    gint node_kind = SIDEBAR_NODE_UNKNOWN;
    char *conn_id = NULL;
    char *db_name = NULL;

    gtk_tree_model_get(model,
                       &iter,
                       SIDEBAR_COL_NODE_KIND, &node_kind,
                       SIDEBAR_COL_CONN_ID, &conn_id,
                       SIDEBAR_COL_DB_NAME, &db_name,
                       -1);

    if (!conn_id || strlen(conn_id) == 0) {
        apply_connection_context(area, NULL, NULL);
    } else if (node_kind == SIDEBAR_NODE_CONNECTION || node_kind == SIDEBAR_NODE_DATABASE || node_kind == SIDEBAR_NODE_OTHER) {
        apply_connection_context(area, conn_id, db_name);
    } else {
        apply_connection_context(area, NULL, NULL);
    }

    g_free(conn_id);
    g_free(db_name);
}

static void content_area_free(gpointer data) {
    ContentArea *area = (ContentArea *)data;
    if (!area) return;

    g_free(area->active_conn_id);
    g_free(area->active_db);
    if (area->query_by_conn) {
        g_hash_table_destroy(area->query_by_conn);
    }
    g_free(area);
}

GtkWidget* create_content_area(void) {
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(content, "content-area");
    gtk_widget_set_hexpand(content, TRUE);
    gtk_widget_set_vexpand(content, TRUE);

    gtk_widget_set_margin_top(content, 10);
    gtk_widget_set_margin_bottom(content, 10);
    gtk_widget_set_margin_start(content, 10);
    gtk_widget_set_margin_end(content, 10);

    ContentArea *area = g_new0(ContentArea, 1);
    area->root = content;
    area->query_by_conn = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    GtkWidget *title = gtk_label_new("SQL Editor (PostgreSQL)");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    area->title_label = title;

    GtkWidget *editor_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(editor_scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(editor_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *editor = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(editor), TRUE);
    gtk_widget_set_hexpand(editor, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(editor_scroll), editor);
    area->editor = editor;

    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_editor_key_pressed), area);
    gtk_widget_add_controller(editor, key_controller);

    GtkWidget *run_button = gtk_button_new_with_label("Run");
    area->run_button = run_button;
    gtk_widget_set_halign(run_button, GTK_ALIGN_START);
    gtk_widget_set_sensitive(run_button, FALSE);
    g_signal_connect(run_button, "clicked", G_CALLBACK(on_run_clicked), area);

    GtkWidget *status = gtk_label_new("Select a connection/database from sidebar to start querying.");
    gtk_widget_set_halign(status, GTK_ALIGN_START);
    area->status_label = status;

    GtkWidget *result_scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(result_scroll, TRUE);
    gtk_widget_set_vexpand(result_scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(result_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *result_tree = gtk_tree_view_new();
    area->result_tree = result_tree;
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(result_scroll), result_tree);

    gtk_box_append(GTK_BOX(content), title);
    gtk_box_append(GTK_BOX(content), editor_scroll);
    gtk_box_append(GTK_BOX(content), run_button);
    gtk_box_append(GTK_BOX(content), status);
    gtk_box_append(GTK_BOX(content), result_scroll);

    g_object_set_data_full(G_OBJECT(content), "content-area-state", area, content_area_free);

    return content;
}

void content_attach_sidebar(GtkWidget *content, Sidebar *sidebar) {
    ContentArea *area = (ContentArea *)g_object_get_data(G_OBJECT(content), "content-area-state");
    if (!area || !sidebar) return;

    area->sidebar = sidebar;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sidebar->treeview));
    g_signal_connect(selection, "changed", G_CALLBACK(on_sidebar_selection_changed), area);
}
