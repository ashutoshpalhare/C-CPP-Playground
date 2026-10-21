# C Projects

A structured roadmap of **18 C projects** across beginner, intermediate, and advanced levels.

> **Current status:** All 18 C projects now have source code and a project README, and every one is marked **In Progress**. *In Progress* means the source has been added but has **not yet been compiled and tested by the repository owner**; no project is marked Completed. Native C programs are compiled and run locally, never by GitHub Actions.

## Beginner

| Project | Main concepts | Status |
| --- | --- | --- |
| [Calculator](./Calculator/README.md) | Functions, switch statements, input validation | In Progress |
| [Number Guessing Game](./Number-Guessing-Game/README.md) | Random numbers, loops, conditionals | In Progress |
| [Unit Converter](./Unit-Converter/README.md) | Functions, menus, arithmetic | In Progress |
| [Student Grade Calculator](./Student-Grade-Calculator/README.md) | Arrays, conditions, functions | In Progress |
| [Simple Interest Calculator](./Simple-Interest-Calculator/README.md) | Variables, formulas, input/output | In Progress |
| [Rock Paper Scissors](./Rock-Paper-Scissors/README.md) | Randomness, loops, game logic | In Progress |

## Intermediate

| Project | Main concepts | Status |
| --- | --- | --- |
| [Tic-Tac-Toe](./Tic-Tac-Toe/README.md) | Arrays, functions, game logic | In Progress |
| [Student Management System](./Student-Management-System/README.md) | Structs, files, record management | In Progress |
| [Contact Book](./Contact-Book/README.md) | Structs, searching, file I/O | In Progress |
| [Expense Tracker](./Expense-Tracker/README.md) | Structs, files, aggregation | In Progress |
| [Quiz Application](./Quiz-Application/README.md) | Arrays, structs, scoring | In Progress |
| [Inventory Management System](./Inventory-Management-System/README.md) | Structs, files, searching | In Progress |

## Advanced

| Project | Main concepts | Status |
| --- | --- | --- |
| [Phonebook Application](./Phonebook-Application/README.md) | Pointers, sorting, file design | In Progress |
| [Basic HTTP Server](./Basic-HTTP-Server/README.md) | Sockets, HTTP, networking | In Progress |
| [Mini Shell](./Mini-Shell/README.md) | Processes, fork/exec, signals | In Progress |
| [File Compression Tool](./File-Compression-Tool/README.md) | Bit operations, Huffman coding, files | In Progress |
| [Terminal Text Editor](./Terminal-Text-Editor/README.md) | Terminal I/O, buffers, state | In Progress |
| [Memory Allocator Simulator](./Memory-Allocator-Simulator/README.md) | Pointers, free lists, fragmentation | In Progress |

## Build and run

Every project has a `main.c` and its own README with exact instructions. The general pattern, from inside a project folder, is:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o app
./app            # Windows: app.exe
```

Some projects need extra flags or a specific platform. Check each README before building:

| Project | Platform / flags |
| --- | --- |
| [Calculator](./Calculator/README.md) | Add `-lm` when linking |
| [Basic HTTP Server](./Basic-HTTP-Server/README.md) | Windows (MinGW-w64): add `-lws2_32`. Also builds on Linux, macOS, and WSL |
| [Mini Shell](./Mini-Shell/README.md) | **POSIX only** — Linux, macOS, or WSL. Not native Windows |
| [Terminal Text Editor](./Terminal-Text-Editor/README.md) | **POSIX terminal only** — Linux, macOS, or WSL. Not native Windows |

All other projects use standard C and are intended to build on Windows, Linux, and macOS. That is the intent of the code; it has not been verified yet.

Source is being added first; compiling, running, and debugging happen locally afterwards.

## Implementation checklist

- [ ] Compile with warnings enabled and resolve relevant warnings.
- [ ] Validate input and handle errors safely.
- [ ] Test typical, invalid, and boundary inputs.
- [ ] Document features, usage, build steps, and known limitations.
- [ ] Update the website status only when the repository reflects the change.

See the [root README](../README.md) for repository-wide guidelines and the [project directory](../index.html) for search and filters.
