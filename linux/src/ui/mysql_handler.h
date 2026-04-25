#ifndef MYSQL_HANDLER_H
#define MYSQL_HANDLER_H
#include <gtk/gtk.h>
#include <mysql.h>
#include "dialog_connection.h"
#include "sidebar.h"

void connect_mysql(SqlConnectContext *ctx);
void load_saved_mysql_connection(Sidebar *sidebar, ConnectionInfo *info);

#endif
    