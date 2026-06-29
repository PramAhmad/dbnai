#include "sidebar.h"
#include "../db/mysql.h"
#include "../db/postgres.h"
#include "../db/sqlite.h"
#include "button.h"
#include "connection_store.h"
#include "dialog_connection.h"

#define CLICKED "clicked"

typedef struct {
  GtkTreeStore *store;
  GList *allocated_iters;
} CallbackCtx;

static void *gtk_tree_callback(void *user_data, void *parent_handle,
                               NodeKind kind, const char *label,
                               const char *conn_id, const char *db_name,
                               const char *schema_name,
                               const char *table_name) {
  CallbackCtx *ctx = (CallbackCtx *)user_data;
  GtkTreeIter *parent_iter = (GtkTreeIter *)parent_handle;
  GtkTreeIter *child_iter = g_malloc(sizeof(GtkTreeIter));
  ctx->allocated_iters = g_list_prepend(ctx->allocated_iters, child_iter);

  gtk_tree_store_append(ctx->store, child_iter, parent_iter);
  gtk_tree_store_set(ctx->store, child_iter, SIDEBAR_COL_LABEL, label,
                     SIDEBAR_COL_NODE_KIND, (int)kind, SIDEBAR_COL_CONN_ID,
                     conn_id, SIDEBAR_COL_DB_NAME, db_name, -1);
  return child_iter;
}

void sidebar_add_connection(Sidebar *sidebar, ConnectionInfo *info) {
  if (!sidebar || !info)
    return;

  GtkTreeIter conn_iter;
  gtk_tree_store_append(sidebar->store, &conn_iter, NULL);
  gtk_tree_store_set(sidebar->store, &conn_iter, SIDEBAR_COL_LABEL, info->name,
                     SIDEBAR_COL_NODE_KIND, NODE_CONNECTION,
                     SIDEBAR_COL_CONN_ID, info->id, SIDEBAR_COL_DB_NAME, "",
                     -1);

  CallbackCtx ctx;
  ctx.store = sidebar->store;
  ctx.allocated_iters = NULL;

  if (info->type == DB_TYPE_POSTGRESQL) {
    pg_load_tree(info, gtk_tree_callback, &ctx, &conn_iter);
  } else if (info->type == DB_TYPE_MYSQL) {
    mysql_load_tree(info, gtk_tree_callback, &ctx, &conn_iter);
  } else if (info->type == DB_TYPE_SQLITE) {
    sqlite_load_tree(info, gtk_tree_callback, &ctx, &conn_iter);
  }

  g_list_free_full(ctx.allocated_iters, g_free);
}

GtkWidget *create_sidebar(void) {
  Sidebar *sidebar = g_malloc(sizeof(Sidebar));

  sidebar->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_size_request(sidebar->container, 200, -1);
  gtk_widget_add_css_class(sidebar->container, "sidebar");

  GtkWidget *btn_create = create_menu_button("Create Connection", NULL);
  gtk_box_append(GTK_BOX(sidebar->container), btn_create);

  sidebar->store = gtk_tree_store_new(SIDEBAR_N_COLS, G_TYPE_STRING, G_TYPE_INT,
                                      G_TYPE_STRING, G_TYPE_STRING);
  sidebar->treeview =
      gtk_tree_view_new_with_model(GTK_TREE_MODEL(sidebar->store));

  sidebar->conn_store = connection_store_new();
  connection_store_load(sidebar->conn_store);

  // Load saved connections
  for (GList *l = sidebar->conn_store->connections; l != NULL; l = l->next) {
    ConnectionInfo *info = (ConnectionInfo *)l->data;
    sidebar_add_connection(sidebar, info);
  }

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
      "Connections", renderer, "text", SIDEBAR_COL_LABEL, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(sidebar->treeview), column);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER,
                                 GTK_POLICY_AUTOMATIC);

  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), sidebar->treeview);

  gtk_box_append(GTK_BOX(sidebar->container), scroll);

  g_object_set_data_full(G_OBJECT(sidebar->container), "sidebar", sidebar,
                         g_free);

  g_signal_connect(btn_create, CLICKED,
                   G_CALLBACK(show_create_connection_dialog), sidebar);

  return sidebar->container;
}

Sidebar *sidebar_from_widget(GtkWidget *container) {
  if (!container)
    return NULL;
  return (Sidebar *)g_object_get_data(G_OBJECT(container), "sidebar");
}
