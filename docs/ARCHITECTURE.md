# DBMRAP Architecture & Clean Code Guide

This document describes the project structure, architecture, and coding standards for DBMRAP.

---

# 1. About DBMRAP

**DBMRAP** is a native, cross-platform database client for Linux, Windows, and macOS.

The project focuses on:

* High performance
* Native look and feel
* No web-based frameworks (Electron, Tauri, etc.)

### Technologies

| Platform | UI              |
| -------- | --------------- |
| Linux    | C + GTK         |
| Windows  | C + Win32 API   |
| macOS    | Swift + SwiftUI |

---

# 2. Project Structure

```text
dbmrap/
├── docs/
│   └── ARCHITECTURE.md
├── linux/
│   ├── src/
│   │   ├── db/
│   │   ├── style/
│   │   └── ui/
│   └── Makefile
├── windows/
│   ├── src/
│   │   ├── db/
│   │   └── ui/
│   ├── Makefile.mingw
│   └── Makefile.msvc
├── macos/
├── LICENSE
└── README.md
```

---

# 3. Architecture

DBMRAP uses a **Decoupled Driver/Adapter** architecture.
The database driver only handles database operations.
The UI only handles rendering.
The two layers are completely independent.

## Design Principles

* Separate database logic from UI logic.
* Drivers must not depend on any UI library.
* Database code should be reusable across all platforms.
* SQL logic should be implemented once and shared.

## Architecture

```text
GTK UI      \
             \
Win32 UI -----> Core DB API -----> PostgreSQL Driver
             /                    MySQL Driver
SwiftUI ----/
```


## UI Layer

Each platform renders the returned data using its native toolkit.
Linux uses GTK.
Windows uses Win32 Common Controls.
macOS uses SwiftUI.

---

# 4. Code Standards

## Naming

* Functions: `snake_case`
* Variables: `snake_case`
* Structs: `PascalCase`
* Enums: `PascalCase`
* Macros: `UPPER_SNAKE_CASE`

Use prefixes to avoid global name collisions.

Examples:

```text
pg_connect()
mysql_connect()
sqlite_connect()
ui_create_window()
```

---

## Memory Management

Always check memory allocation.

```c
ConnectionInfo *info = malloc(sizeof(ConnectionInfo));

if (!info) {
    return NULL;
}
```

Every allocation function should have a matching free function.

Example:

```text
connection_store_new()
connection_store_free()
```

Use a safe free macro.

```c
#define SAFE_FREE(ptr) \
do {                   \
    free(ptr);         \
    ptr = NULL;        \
} while (0)
```

---

## Error Handling

Functions that can fail should return a status code.

Example:

```c
BOOL db_connection_test(
    ConnectionInfo *info,
    char *error,
    size_t length
);
```

For multiple resources, use a cleanup section.

```c
FILE *file = fopen(path, "r");
if (!file)
    return -1;

char *buffer = malloc(1024);
if (!buffer)
    goto cleanup;

/* Process */

cleanup:
    free(buffer);
    fclose(file);
```

---

## Header Files

Every header must use include guards.

```c
#ifndef DB_API_H
#define DB_API_H

/* declarations */

#endif
```

Avoid including platform-specific headers in generic headers.

Do **not** include:

* `windows.h`
* `gtk/gtk.h`
* Swift headers

Generic headers should compile on every supported platform.

---

