CC = gcc
CFLAGS = `pkg-config --cflags gtk4` `mariadb_config --cflags` -I/usr/local/MATLAB/R2025b/sys/postgresql/glnxa64/PostgreSQL/include -Wall -Wextra -std=c11
LDFLAGS = -L/usr/local/MATLAB/R2025b/sys/postgresql/glnxa64/PostgreSQL/lib -Wl,-rpath,/usr/local/MATLAB/R2025b/sys/postgresql/glnxa64/PostgreSQL/lib -lpq `mariadb_config --libs` -l:libsqlite3.so.0 `pkg-config --libs gtk4`

TARGET = dbnai
SOURCES = src/main.c src/ui/navbar.c src/ui/sidebar.c src/ui/content.c src/ui/button.c src/ui/dialog_connection.c src/style/css.c src/db/postgres.c src/db/mysql.c src/db/sqlite.c src/ui/connection_store.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -Isrc $(SOURCES) -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
