# C++ Projects

A structured collection of **18 C++ projects** across beginner, intermediate, and advanced levels.

> **Current status:** All 18 projects have an initial `main.cpp` implementation and a project README. Every project is marked **In Progress** because source code has not yet been compiled and tested by the repository owner. Complete the local verification checklist before marking any project Completed.

## Beginner

| Project | Main concepts | Status |
| --- | --- | --- |
| [Quiz Game](./Quiz-Game/README.md) | Classes, vectors, control flow | In Progress |
| [Task Manager App](./Task-Manager-App/README.md) | Classes, vectors, sorting | In Progress |
| [To-Do List](./To-Do-List/README.md) | STL, vectors, functions | In Progress |
| [Password Generator](./Password-Generator/README.md) | Strings, random numbers, validation | In Progress |
| [Digital Clock](./Digital-Clock/README.md) | Chrono, time formatting, loops | In Progress |
| [Bank Account Simulator](./Bank-Account-Simulator/README.md) | Classes, encapsulation, validation | In Progress |

## Intermediate

| Project | Main concepts | Status |
| --- | --- | --- |
| [Library Management System](./Library-Management-System/README.md) | OOP, STL containers, files | In Progress |
| [Bank Management System](./Bank-Management-System/README.md) | Classes, transactions, persistence | In Progress |
| [Snake Game](./Snake-Game/README.md) | Game loop, state, collision | In Progress |
| [Employee Payroll System](./Employee-Payroll-System/README.md) | Inheritance, polymorphism, files | In Progress |
| [Parking Lot Management System](./Parking-Lot-Management-System/README.md) | OOP, maps, time | In Progress |
| [Tic-Tac-Toe with AI](./Tic-Tac-Toe-with-AI/README.md) | Recursion, minimax, game trees | In Progress |

## Advanced

| Project | Main concepts | Status |
| --- | --- | --- |
| [Custom String Class](./Custom-String-Class/README.md) | RAII, Rule of Five, operators | In Progress |
| [Chess Game](./Chess-Game/README.md) | OOP, state modelling, validation | In Progress |
| [Chat Application using Sockets](./Chat-Application-using-Sockets/README.md) | Sockets, threads, protocols | In Progress |
| [Mini Database Engine](./Mini-Database-Engine/README.md) | Parsing, B-trees, storage | In Progress |
| [Memory Pool Allocator](./Memory-Pool-Allocator/README.md) | Memory management, alignment, RAII | In Progress |
| [Multithreaded Download Manager](./Multithreaded-Download-Manager/README.md) | Threads, synchronisation, I/O | In Progress |

## Build and run

Each project now includes a `main.cpp` source file and its own README with compiler requirements, build commands, usage notes, known limitations, and a testing checklist. These instructions have not yet been verified by compiling and running the programs locally.

Typical G++ example for a future single-file project:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o app
./app
```

## Implementation checklist

- [ ] Use clear types and focused class/function responsibilities.
- [ ] Prefer standard library facilities where appropriate.
- [ ] Compile with warnings enabled and resolve relevant warnings.
- [ ] Handle invalid input and resource lifetimes carefully.
- [ ] Test typical, invalid, and boundary cases.
- [ ] Document features, usage, build steps, and known limitations.
- [ ] Update the website status only when the repository reflects the change.

See the [root README](../README.md) for repository-wide guidelines and the [project directory](../index.html) for search and filters.
