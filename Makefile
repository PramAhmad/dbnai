CC = gcc
CFLAGS = -Wall -Wextra -std=c11 `pkg-config --cflags gtk4`
LDFLAGS = `pkg-config --libs gtk4`

TARGET = dbnai
SOURCES = src/main.c src/ui/navbar.c src/ui/sidebar.c src/ui/content.c src/ui/button.c src/ui/dialog_connection.c src/style/css.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -Isrc $(SOURCES) -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)


