### DBMRAP
DBmrap Is a native database client tool for Linux, Windows, and macOS. We prioritize ease of use and efficiency with high performance on every platform. DBmrap supports various database systems, providing a seamless experience for developers and database administrators. With its intuitive interface and powerful features, DBmrap is the go-to solution for managing your databases effectively across different operating systems.

## Linux
We built DBmrap using Gtk, a popular toolkit for creating graphical user interfaces on Linux. This choice allows us to provide a native look and feel while ensuring high performance and responsiveness. DBmrap is optimized for Linux environments, making it an ideal choice for developers and database administrators who prefer working on this platform.

## Windows
For Windows, we developed DBmrap using the Windows API, which allows us to create a native application that integrates seamlessly with the Windows operating system. This approach ensures that DBmrap delivers a smooth and efficient user experience on Windows, making it a powerful tool for managing databases on this platform.

## macOS
On macOS, we utilized SwiftUI to build DBmrap, providing a modern and intuitive user interface that takes advantage of the latest features of the macOS platform. This allows us to offer a native experience for macOS users while maintaining high performance and efficiency in managing databases.

## Project Goals

DBMRAP aims to provide a fast, lightweight, and truly native database client for modern desktop operating systems. As computer hardware continues to become more powerful, many desktop applications have also become increasingly large and memory-intensive, often relying on heavyweight runtime environments for relatively simple tasks.

This project takes a different approach. DBMRAP focuses on efficient resource usage, native performance, and minimal dependencies. By using each platform's native UI framework and a modular database architecture, the application remains responsive, portable, and easy to maintain without unnecessary software bloat.

The project also emphasizes clean architecture, reusable database drivers, and maintainable code, making it suitable for long-term development and future platform expansion

## Documentation
For information on the codebase structure, design patterns (Linux/Windows decoupling), and C Clean Code guidelines, please refer to:
*   [Architecture & Clean Code Guidelines (docs/ARCHITECTURE.md)](docs/ARCHITECTURE.md)