CC = gcc
CFLAGS = `pkg-config --cflags gtk4` `mariadb_config --cflags` -Wall -Wextra -std=c11
LDFLAGS = -lpq `mariadb_config --libs` `pkg-config --libs gtk4`

TARGET = dbnai
SOURCES = src/main.c src/ui/navbar.c src/ui/sidebar.c src/ui/content.c src/ui/button.c src/ui/dialog_connection.c src/style/css.c src/ui/postgres.c src/ui/mysql_handler.c src/ui/connection_store.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -Isrc $(SOURCES) -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)


