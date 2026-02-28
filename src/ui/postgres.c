#include "postgres.h"
#include <string.h>


void validate_pg_form(PgForm *form) {
    FormField fields[] = {
        {form->host, form->err_host, "Host wajib diisi", "host"},
        {form->port, form->err_port, "Port wajib diisi", "port"},
        {form->username, form->err_username, "Username wajib diisi", "username"},
        {form->password, form->err_password, "Password wajib diisi", "password"}
    };

    for (int i = 0; i < G_N_ELEMENTS(fields); i++) {
        const char *text = gtk_editable_get_text(GTK_EDITABLE(fields[i].entry));
        
        if (strlen(text) == 0) {
            gtk_label_set_text(GTK_LABEL(fields[i].err_label), fields[i].err_msg);
            gtk_widget_set_visible(fields[i].err_label, TRUE);
        } else {
            gtk_widget_set_visible(fields[i].err_label, FALSE);
        }
    }
}


static PGconn *pg_connect(const char *host, const char *port, 
                          const char *username, const char *password, 
                          const char *dbname) {
    char conninfo[512];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s user=%s password=%s dbname=%s",
             host, port, username, password, dbname);
    return PQconnectdb(conninfo);
}


static void load_columns(PGconn *conn, GtkTreeStore *store, 
                         GtkTreeIter *parent, const char *schema, 
                         const char *table) {
    GtkTreeIter col_parent;
    gtk_tree_store_append(store, &col_parent, parent);
    gtk_tree_store_set(store, &col_parent, 0, "Columns", -1);

    char query[512];
    snprintf(query, sizeof(query),
        "SELECT column_name, data_type, character_maximum_length, numeric_precision "
        "FROM information_schema.columns "
        "WHERE table_schema='%s' AND table_name='%s' "
        "ORDER BY ordinal_position;",
        schema, table);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        for (int i = 0; i < PQntuples(res); i++) {
            const char *col_name = PQgetvalue(res, i, 0);
            const char *data_type = PQgetvalue(res, i, 1);
            const char *char_len = PQgetvalue(res, i, 2);
            const char *num_prec = PQgetvalue(res, i, 3);

            char col_display[256];
            if (strlen(char_len) > 0) {
                snprintf(col_display, sizeof(col_display), "%s (%s(%s))", 
                         col_name, data_type, char_len);
            } else if (strlen(num_prec) > 0) {
                snprintf(col_display, sizeof(col_display), "%s (%s(%s))", 
                         col_name, data_type, num_prec);
            } else {
                snprintf(col_display, sizeof(col_display), "%s (%s)", 
                         col_name, data_type);
            }

            GtkTreeIter col_iter;
            gtk_tree_store_append(store, &col_iter, &col_parent);
            gtk_tree_store_set(store, &col_iter, 0, col_display, -1);
        }
    }
    PQclear(res);
}


static void load_foreign_keys(PGconn *conn, GtkTreeStore *store, 
                              GtkTreeIter *parent, const char *schema, 
                              const char *table) {
    GtkTreeIter fk_parent;
    gtk_tree_store_append(store, &fk_parent, parent);
    gtk_tree_store_set(store, &fk_parent, 0, "Foreign Keys", -1);

    char query[1024];
    snprintf(query, sizeof(query),
        "SELECT "
        "    kcu.constraint_name, "
        "    kcu.column_name, "
        "    ccu.table_schema AS ref_schema, "
        "    ccu.table_name AS ref_table, "
        "    ccu.column_name AS ref_column "
        "FROM information_schema.table_constraints tc "
        "JOIN information_schema.key_column_usage kcu "
        "    ON tc.constraint_name = kcu.constraint_name "
        "    AND tc.table_schema = kcu.table_schema "
        "JOIN information_schema.constraint_column_usage ccu "
        "    ON ccu.constraint_name = tc.constraint_name "
        "WHERE tc.constraint_type = 'FOREIGN KEY' "
        "    AND tc.table_schema = '%s' "
        "    AND tc.table_name = '%s';",
        schema, table);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        for (int i = 0; i < PQntuples(res); i++) {
            const char *fk_name = PQgetvalue(res, i, 0);
            const char *col_name = PQgetvalue(res, i, 1);
            const char *ref_schema = PQgetvalue(res, i, 2);
            const char *ref_table = PQgetvalue(res, i, 3);
            const char *ref_col = PQgetvalue(res, i, 4);

            char fk_display[512];
            snprintf(fk_display, sizeof(fk_display), "%s: %s -> %s.%s.%s",
                     fk_name, col_name, ref_schema, ref_table, ref_col);

            GtkTreeIter fk_iter;
            gtk_tree_store_append(store, &fk_iter, &fk_parent);
            gtk_tree_store_set(store, &fk_iter, 0, fk_display, -1);
        }
    }
    PQclear(res);
}


static void load_tables(PGconn *conn, GtkTreeStore *store, 
                        GtkTreeIter *schema_iter, const char *schema) {
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT table_name FROM information_schema.tables "
             "WHERE table_schema='%s';",
             schema);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        for (int t = 0; t < PQntuples(res); t++) {
            const char *table_name = PQgetvalue(res, t, 0);

            GtkTreeIter table_iter;
            gtk_tree_store_append(store, &table_iter, schema_iter);
            gtk_tree_store_set(store, &table_iter, 0, table_name, -1);

            load_columns(conn, store, &table_iter, schema, table_name);
            load_foreign_keys(conn, store, &table_iter, schema, table_name);
        }
    }
    PQclear(res);
}


static void load_schemas(PGconn *conn, GtkTreeStore *store, 
                         GtkTreeIter *db_iter) {
    PGresult *res = PQexec(conn,
        "SELECT schema_name FROM information_schema.schemata "
        "WHERE schema_name NOT LIKE 'pg_%' "
        "AND schema_name != 'information_schema';");

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        for (int s = 0; s < PQntuples(res); s++) {
            const char *schema_name = PQgetvalue(res, s, 0);

            GtkTreeIter schema_iter;
            gtk_tree_store_append(store, &schema_iter, db_iter);
            gtk_tree_store_set(store, &schema_iter, 0, schema_name, -1);

            load_tables(conn, store, &schema_iter, schema_name);
        }
    }
    PQclear(res);
}


static void load_database(GtkTreeStore *store, GtkTreeIter *db_iter,
                          const char *host, const char *port,
                          const char *username, const char *password,
                          const char *dbname) {
    PGconn *db_conn = pg_connect(host, port, username, password, dbname);
    if (PQstatus(db_conn) != CONNECTION_OK) {
        g_print("Failed to connect to %s: %s\n", dbname, PQerrorMessage(db_conn));
        PQfinish(db_conn);
        return;
    }

    load_schemas(db_conn, store, db_iter);
    PQfinish(db_conn);
}

void get_tree_databases(PGconn *conn, Sidebar *sidebar) {
    PGresult *res = PQexec(conn, "SELECT datname FROM pg_database WHERE datistemplate = false;");
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        gtk_tree_store_clear(sidebar->store);
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            const char *db = PQgetvalue(res, i, 0);
            GtkTreeIter db_iter;
            gtk_tree_store_append(sidebar->store, &db_iter, NULL);
            gtk_tree_store_set(sidebar->store, &db_iter, 0, db, -1);
        }
    }
    PQclear(res);
}


void connect_postgresql(PgConnectContext *ctx) {
    PgForm *form = ctx->form;
    const char *host = gtk_editable_get_text(GTK_EDITABLE(form->host));
    const char *port = gtk_editable_get_text(GTK_EDITABLE(form->port));
    const char *username = gtk_editable_get_text(GTK_EDITABLE(form->username));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(form->password));
    const char *db_name = gtk_editable_get_text(GTK_EDITABLE(form->db));

    validate_pg_form(form);

    const char *initial_db = (strlen(db_name) == 0) ? "postgres" : db_name;

    PGconn *conn = pg_connect(host, port, username, password, initial_db);
    if (PQstatus(conn) != CONNECTION_OK) {
        g_print("Connection failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return;
    }

    PGresult *res;
    if (strlen(db_name) == 0) {
        res = PQexec(conn, "SELECT datname FROM pg_database WHERE datistemplate = false;");
    } else {
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

            load_database(ctx->sidebar->store, &db_iter,
                         host, port, username, password, db);
        }
    }

    PQclear(res);
    PQfinish(conn);
}