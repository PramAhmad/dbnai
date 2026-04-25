#include "mysql_handler.h"
#include "connection_store.h"
#include <string.h>


static MYSQL *mysql_connect(const char *host, const char *port, 
                            const char *username, const char *password, 
                            const char *dbname) {
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        g_print("mysql_init failed\n");
        return NULL;
    }

    unsigned int port_num = (strlen(port) > 0) ? atoi(port) : 3306;
    
    if (!mysql_real_connect(conn, host, username, password, 
                           (strlen(dbname) > 0) ? dbname : "mysql",
                           port_num, NULL, 0)) {
        g_print("MySQL connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}



static void load_mysql_columns(MYSQL *conn, GtkTreeStore *store,
                               GtkTreeIter *parent, const char *db_name,
                               const char *table_name) {
    GtkTreeIter col_parent;
    gtk_tree_store_append(store, &col_parent, parent);
    gtk_tree_store_set(store, &col_parent, 0, "Columns", -1);

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT COLUMN_NAME, COLUMN_TYPE, CHARACTER_MAXIMUM_LENGTH, NUMERIC_PRECISION "
             "FROM INFORMATION_SCHEMA.COLUMNS "
             "WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='%s' "
             "ORDER BY ORDINAL_POSITION;",
             db_name, table_name);

    if (mysql_query(conn, query) != 0) {
        g_print("MySQL query failed: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        g_print("mysql_store_result failed\n");
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL) {
        const char *col_name = row[0];
        const char *data_type = row[1];
        const char *char_len = row[2] ? row[2] : "";
        const char *num_prec = row[3] ? row[3] : "";

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

    mysql_free_result(result);
}



static void load_mysql_foreign_keys(MYSQL *conn, GtkTreeStore *store,
                                    GtkTreeIter *parent, const char *db_name,
                                    const char *table_name) {
    GtkTreeIter fk_parent;
    gtk_tree_store_append(store, &fk_parent, parent);
    gtk_tree_store_set(store, &fk_parent, 0, "Foreign Keys", -1);

    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT CONSTRAINT_NAME, COLUMN_NAME, REFERENCED_TABLE_SCHEMA, "
             "REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME "
             "FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE "
             "WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='%s' "
             "AND REFERENCED_TABLE_NAME IS NOT NULL;",
             db_name, table_name);

    if (mysql_query(conn, query) != 0) {
        g_print("MySQL FK query failed: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL) {
        const char *fk_name = row[0];
        const char *col_name = row[1];
        const char *ref_schema = row[2];
        const char *ref_table = row[3];
        const char *ref_col = row[4];

        char fk_display[512];
        snprintf(fk_display, sizeof(fk_display), "%s: %s -> %s.%s.%s",
                 fk_name, col_name, ref_schema, ref_table, ref_col);

        GtkTreeIter fk_iter;
        gtk_tree_store_append(store, &fk_iter, &fk_parent);
        gtk_tree_store_set(store, &fk_iter, 0, fk_display, -1);
    }

    mysql_free_result(result);
}



static void load_mysql_tables(MYSQL *conn, GtkTreeStore *store,
                              GtkTreeIter *db_iter, const char *db_name) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES "
             "WHERE TABLE_SCHEMA='%s' AND TABLE_TYPE='BASE TABLE';",
             db_name);

    if (mysql_query(conn, query) != 0) {
        g_print("MySQL tables query failed: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL) {
        const char *table_name = row[0];

        GtkTreeIter table_iter;
        gtk_tree_store_append(store, &table_iter, db_iter);
        gtk_tree_store_set(store, &table_iter, 0, table_name, -1);

        load_mysql_columns(conn, store, &table_iter, db_name, table_name);
        load_mysql_foreign_keys(conn, store, &table_iter, db_name, table_name);
    }

    mysql_free_result(result);
}



static void load_mysql_database(GtkTreeStore *store, GtkTreeIter *db_iter,
                                const char *host, const char *port,
                                const char *username, const char *password,
                                const char *dbname) {
    MYSQL *conn = mysql_connect(host, port, username, password, dbname);
    if (!conn) {
        g_print("Failed to connect to MySQL database %s\n", dbname);
        return;
    }

    load_mysql_tables(conn, store, db_iter, dbname);
    mysql_close(conn);
}



void connect_mysql(SqlConnectContext *ctx) {
    SqlForm *form = ctx->form;
    const char *conn_name = gtk_editable_get_text(GTK_EDITABLE(form->name));
    const char *host = gtk_editable_get_text(GTK_EDITABLE(form->host));
    const char *port = gtk_editable_get_text(GTK_EDITABLE(form->port));
    const char *username = gtk_editable_get_text(GTK_EDITABLE(form->username));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(form->password));
    const char *db_name = gtk_editable_get_text(GTK_EDITABLE(form->db));

    if (strlen(conn_name) == 0 || strlen(host) == 0 || 
        strlen(port) == 0 || strlen(username) == 0 || strlen(password) == 0) {
        return;
    }

    const char *initial_db = (strlen(db_name) == 0) ? "mysql" : db_name;

    MYSQL *conn = mysql_connect(host, port, username, password, initial_db);
    if (!conn) {
        g_print("Connection failed to MySQL\n");
        return;
    }

    // Save connection to store
    if (ctx->conn_store) {
        ConnectionInfo *info = connection_info_new(
            conn_name, DB_TYPE_MYSQL,
            host, port, username, password, db_name
        );
        connection_store_add(ctx->conn_store, info);
        connection_store_save(ctx->conn_store);
    }

    // Create connection root node
    GtkTreeIter conn_iter;
    gtk_tree_store_append(ctx->sidebar->store, &conn_iter, NULL);
    gtk_tree_store_set(ctx->sidebar->store, &conn_iter, 0, conn_name, -1);

    // Get databases list
    char query_str[256];
    if (strlen(db_name) == 0) {
        snprintf(query_str, sizeof(query_str), "SHOW DATABASES;");
    } else {
        snprintf(query_str, sizeof(query_str), "SELECT DATABASE();");
    }

    if (mysql_query(conn, query_str) != 0) {
        g_print("MySQL query failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        mysql_close(conn);
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL) {
        const char *db = row[0];

        // Skip system databases
        if (strcmp(db, "mysql") == 0 || strcmp(db, "information_schema") == 0 || 
            strcmp(db, "performance_schema") == 0 || strcmp(db, "sys") == 0) {
            continue;
        }

        GtkTreeIter db_iter;
        gtk_tree_store_append(ctx->sidebar->store, &db_iter, &conn_iter);
        gtk_tree_store_set(ctx->sidebar->store, &db_iter, 0, db, -1);

        load_mysql_database(ctx->sidebar->store, &db_iter,
                           host, port, username, password, db);
    }

    mysql_free_result(result);
    mysql_close(conn);
}


void load_saved_mysql_connection(Sidebar *sidebar, ConnectionInfo *info) {
    if (!info || info->type != DB_TYPE_MYSQL) return;

    const char *initial_db = (strlen(info->database) == 0) ? "mysql" : info->database;

    MYSQL *conn = mysql_connect(info->host, info->port, info->username,
                               info->password, initial_db);
    if (!conn) {
        g_print("Failed to restore MySQL connection %s\n", info->name);
        return;
    }

    // Create connection root node
    GtkTreeIter conn_iter;
    gtk_tree_store_append(sidebar->store, &conn_iter, NULL);
    gtk_tree_store_set(sidebar->store, &conn_iter, 0, info->name, -1);

    // Get databases list
    char query_str[256];
    if (strlen(info->database) == 0) {
        snprintf(query_str, sizeof(query_str), "SHOW DATABASES;");
    } else {
        snprintf(query_str, sizeof(query_str), "SELECT DATABASE();");
    }

    if (mysql_query(conn, query_str) != 0) {
        mysql_close(conn);
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        mysql_close(conn);
        return;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL) {
        const char *db = row[0];

        // Skip system databases
        if (strcmp(db, "mysql") == 0 || strcmp(db, "information_schema") == 0 ||
            strcmp(db, "performance_schema") == 0 || strcmp(db, "sys") == 0) {
            continue;
        }

        GtkTreeIter db_iter;
        gtk_tree_store_append(sidebar->store, &db_iter, &conn_iter);
        gtk_tree_store_set(sidebar->store, &db_iter, 0, db, -1);

        load_mysql_database(sidebar->store, &db_iter,
                           info->host, info->port, info->username,
                           info->password, db);
    }

    mysql_free_result(result);
    mysql_close(conn);
}
