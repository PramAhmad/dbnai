#ifndef DATABASE_TREE_H
#define DATABASE_TREE_H

#include <gtk/gtk.h>
#include <libpq-fe.h>

typedef struct {
    char *host;
    char *port;
    char *username;
    char *password;
} ConnectionInfo;

GtkWidget* create_database_tree(void);
void add_database_to_tree(GtkWidget *tree, const char *db_name, ConnectionInfo *conn_info);
void populate_tables(GtkTreeStore *store, GtkTreeIter *db_iter, const char *db_name, ConnectionInfo *conn_info);

#endif