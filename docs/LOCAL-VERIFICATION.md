# Local Verification Tracker

This tracker is intentionally evidence-based. **Source files being present does not mean a project compiles or behaves correctly.** Native C/C++ programs are not compiled or executed by GitHub Actions. Perform native checks locally and update this file only with actual results.

## Result definitions

- **Not tested** — no local result has been recorded.
- **Compile passed** — compiler exited successfully with the documented command; runtime behavior is not implied.
- **Runtime checked** — program was run locally for the listed scenarios.
- **Blocked** — a concrete issue prevents the check; include the error and environment.
- **Verified** — required compile and documented behavior checks passed, and limitations are recorded.

## C projects

| Project | Compile | Runtime checks | Environment / notes |
| --- | --- | --- | --- |
| [Calculator](../C/Calculator/README.md) | Not tested | Not tested | Pending local verification |
| [Number Guessing Game](../C/Number-Guessing-Game/README.md) | Not tested | Not tested | Pending local verification |
| [Unit Converter](../C/Unit-Converter/README.md) | Not tested | Not tested | Pending local verification |
| [Student Grade Calculator](../C/Student-Grade-Calculator/README.md) | Not tested | Not tested | Pending local verification |
| [Simple Interest Calculator](../C/Simple-Interest-Calculator/README.md) | Not tested | Not tested | Pending local verification |
| [Rock Paper Scissors](../C/Rock-Paper-Scissors/README.md) | Not tested | Not tested | Pending local verification |
| [Tic-Tac-Toe](../C/Tic-Tac-Toe/README.md) | Not tested | Not tested | Pending local verification |
| [Student Management System](../C/Student-Management-System/README.md) | Not tested | Not tested | Pending local verification |
| [Contact Book](../C/Contact-Book/README.md) | Not tested | Not tested | Pending local verification |
| [Expense Tracker](../C/Expense-Tracker/README.md) | Not tested | Not tested | Pending local verification |
| [Quiz Application](../C/Quiz-Application/README.md) | Not tested | Not tested | Pending local verification |
| [Inventory Management System](../C/Inventory-Management-System/README.md) | Not tested | Not tested | Pending local verification |
| [Phonebook Application](../C/Phonebook-Application/README.md) | Not tested | Not tested | Pending local verification |
| [Basic HTTP Server](../C/Basic-HTTP-Server/README.md) | Not tested | Not tested | Pending local verification |
| [Mini Shell](../C/Mini-Shell/README.md) | Not tested | Not tested | POSIX-only; pending local verification |
| [File Compression Tool](../C/File-Compression-Tool/README.md) | Not tested | Not tested | Pending local verification |
| [Terminal Text Editor](../C/Terminal-Text-Editor/README.md) | Not tested | Not tested | POSIX-terminal-only; pending local verification |
| [Memory Allocator Simulator](../C/Memory-Allocator-Simulator/README.md) | Not tested | Not tested | Pending local verification |

## C++ projects

| Project | Compile | Runtime checks | Environment / notes |
| --- | --- | --- | --- |
| [Quiz Game](../CPP/Quiz-Game/README.md) | Not tested | Not tested | Pending local verification |
| [Task Manager App](../CPP/Task-Manager-App/README.md) | Not tested | Not tested | Pending local verification |
| [To-Do List](../CPP/To-Do-List/README.md) | Not tested | Not tested | Pending local verification |
| [Password Generator](../CPP/Password-Generator/README.md) | Not tested | Not tested | Pending local verification |
| [Digital Clock](../CPP/Digital-Clock/README.md) | Not tested | Not tested | Pending local verification |
| [Bank Account Simulator](../CPP/Bank-Account-Simulator/README.md) | Not tested | Not tested | Pending local verification |
| [Library Management System](../CPP/Library-Management-System/README.md) | Not tested | Not tested | Pending local verification |
| [Bank Management System](../CPP/Bank-Management-System/README.md) | Not tested | Not tested | Pending local verification |
| [Snake Game](../CPP/Snake-Game/README.md) | Not tested | Not tested | Pending local verification |
| [Employee Payroll System](../CPP/Employee-Payroll-System/README.md) | Not tested | Not tested | Pending local verification |
| [Parking Lot Management System](../CPP/Parking-Lot-Management-System/README.md) | Not tested | Not tested | Pending local verification |
| [Tic-Tac-Toe with AI](../CPP/Tic-Tac-Toe-with-AI/README.md) | Not tested | Not tested | Pending local verification |
| [Custom String Class](../CPP/Custom-String-Class/README.md) | Not tested | Not tested | Pending local verification |
| [Chess Game](../CPP/Chess-Game/README.md) | Not tested | Not tested | Pending local verification |
| [Chat Application using Sockets](../CPP/Chat-Application-using-Sockets/README.md) | Not tested | Not tested | Pending local verification |
| [Mini Database Engine](../CPP/Mini-Database-Engine/README.md) | Not tested | Not tested | Pending local verification |
| [Memory Pool Allocator](../CPP/Memory-Pool-Allocator/README.md) | Not tested | Not tested | Pending local verification |
| [Multithreaded Download Manager](../CPP/Multithreaded-Download-Manager/README.md) | Not tested | Not tested | Pending local verification |

## Recommended local workflow

1. Open a terminal in one project folder.
2. Read that project's README first and install the documented compiler/dependencies.
3. Compile with warnings enabled using the project-specific command.
4. Record compiler version, operating system, command, and exact outcome.
5. Run only after reviewing what the program does; use safe test inputs.
6. Check typical, invalid, and boundary cases where relevant.
7. Record limitations and update the website status only when evidence supports it.

Do not mark any row passed without a real local result. GitHub Pages deployment validates the static website delivery, not native program correctness.
