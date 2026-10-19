# C Projects

A structured roadmap of **18 C projects** across beginner, intermediate, and advanced levels.

> **Current status:** Calculator and Number Guessing Game have initial source implementations and are marked **In Progress**. The other 16 C projects currently have planning READMEs only.

## Beginner

| Project | Main concepts | Status |
| --- | --- | --- |
| [Calculator](./Calculator/README.md) | Functions, switch statements, input validation | In Progress |
| [Number Guessing Game](./Number-Guessing-Game/README.md) | Random numbers, loops, conditionals | In Progress |
| [Unit Converter](./Unit-Converter/README.md) | Functions, menus, arithmetic | Planned |
| [Student Grade Calculator](./Student-Grade-Calculator/README.md) | Arrays, conditions, functions | Planned |
| [Simple Interest Calculator](./Simple-Interest-Calculator/README.md) | Variables, formulas, input/output | Planned |
| [Rock Paper Scissors](./Rock-Paper-Scissors/README.md) | Randomness, loops, game logic | Planned |

## Intermediate

| Project | Main concepts | Status |
| --- | --- | --- |
| [Tic-Tac-Toe](./Tic-Tac-Toe/README.md) | 2D arrays, functions, game logic | Planned |
| [Student Management System](./Student-Management-System/README.md) | Structs, files, record management | Planned |
| [Contact Book](./Contact-Book/README.md) | Structs, searching, file I/O | Planned |
| [Expense Tracker](./Expense-Tracker/README.md) | Structs, files, aggregation | Planned |
| [Quiz Application](./Quiz-Application/README.md) | Arrays, structs, scoring | Planned |
| [Inventory Management System](./Inventory-Management-System/README.md) | Structs, files, searching | Planned |

## Advanced

| Project | Main concepts | Status |
| --- | --- | --- |
| [Phonebook Application](./Phonebook-Application/README.md) | Pointers, sorting, file design | Planned |
| [Basic HTTP Server](./Basic-HTTP-Server/README.md) | Sockets, HTTP, networking | Planned |
| [Mini Shell](./Mini-Shell/README.md) | Processes, fork/exec, signals | Planned |
| [File Compression Tool](./File-Compression-Tool/README.md) | Bit operations, Huffman coding, files | Planned |
| [Terminal Text Editor](./Terminal-Text-Editor/README.md) | Terminal I/O, buffers, state | Planned |
| [Memory Allocator Simulator](./Memory-Allocator-Simulator/README.md) | Pointers, free lists, fragmentation | Planned |

## Build and run

The first two C implementations are [Calculator](./Calculator/README.md) and [Number Guessing Game](./Number-Guessing-Game/README.md). Compile each locally using its own README; testing is deferred until the project collection is implemented. The Calculator command is:

```bash
cd Calculator
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o calculator -lm
./calculator
```

For every project, follow its own README for compiler requirements and build commands. We are adding source code first and will compile, run, and debug projects locally after the planned implementations are in place.

## Implementation checklist

- [ ] Compile with warnings enabled and resolve relevant warnings.
- [ ] Validate input and handle errors safely.
- [ ] Test typical, invalid, and boundary inputs.
- [ ] Document features, usage, build steps, and known limitations.
- [ ] Update the website status only when the repository reflects the change.

See the [root README](../README.md) for repository-wide guidelines and the [project directory](../index.html) for search and filters.
