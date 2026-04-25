#include "database_tree.h"

enum {
    COL_NAME,
    COL_TYPE,
    N_COLS,
};

GtkWidget* create_database_tree(void) {
    GtkTreeStore *store = gtk_tree_store_new(N_COLS, G_TYPE_STRING, G_TYPE_INT);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes("Database", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col_name);

    g_object_unref(store);

    return tree;
}

void populate_tables(GtkTreeStore *store, GtkTreeIter *db_iter, const char *db_name, ConnectionInfo *conn_info) {
    char conninfo[512];
    snprintf(conninfo, sizeof(conninfo), "host=%s port=%s user=%s password=%s dbname=%s",
             conn_info->host, conn_info->port, conn_info->username, conn_info->password, db_name);
    PGconn *conn = PQconnectdb(conninfo);
    if(PQstatus(conn) != CONNECTION_OK){
        g_print("failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn); 
        return;
    }

    PGresult *res = PQexec(conn, 
        "SELECT schemaname FROM pg_catalog.pg_namespace WHERE schemaname NOT LIKE 'pg_%' AND schemaname != 'information_schema' ORDER BY schemaname;");
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            const char *schema_name = PQgetvalue(res, i, 0);
            GtkTreeIter schema_iter;
            gtk_tree_store_append(store, &schema_iter, db_iter);
            gtk_tree_store_set(store, &schema_iter, COL_NAME, schema_name, COL_TYPE, 1, -1);
            g_print("  Added schema: %s\n", schema_name);
            char query[512];
            snprintf(query, sizeof(query), "SELECT tablename FROM pg_tables WHERE schemaname = '%s';", schema_name);
            PGresult *table_res = PQexec(conn, query);
            if (PQresultStatus(table_res) == PGRES_TUPLES_OK) {
                int table_rows = PQntuples(table_res);
                for (int j = 0; j < table_rows; j++) {
                    const char *table_name = PQgetvalue(table_res, j, 0);
                    GtkTreeIter table_iter;
                    gtk_tree_store_append(store, &table_iter, &schema_iter);
                    gtk_tree_store_set(store, &table_iter, COL_NAME, table_name, COL_TYPE, 2, -1);
                    g_print("    Added table: %s\n", table_name);
                }
            }
            PQclear(table_res);
        }
    }
    PQclear(res);
    PQfinish(conn);


}

void add_database_to_tree(GtkWidget *tree, const char *db_name, ConnectionInfo *conn_info)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    if (model == NULL) {
        g_print("Error: Tree model is NULL\n");
        return;
    }
    
    GtkTreeStore *store = GTK_TREE_STORE(model);
    
    GtkTreeIter db_iter;
    gtk_tree_store_append(store, &db_iter, NULL);
    gtk_tree_store_set(store, &db_iter, COL_NAME, db_name, COL_TYPE, 0, -1);
    g_print("✓ Database '%s' added to tree model\n", db_name);
    
    // Populate schemas and tables
    populate_tables(store, &db_iter, db_name, conn_info);
    g_print("✓ Finished populating '%s'\n", db_name);
}