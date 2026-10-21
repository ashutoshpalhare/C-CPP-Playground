# C-CPP-Playground

A growing, beginner-to-advanced collection of **C and C++ learning projects**, with a searchable project directory hosted as a static website.

> **Current status:** All 36 C and C++ projects have source implementations and project READMEs, and all are marked **In Progress**. Source code is present, but native programs have not yet been compiled or run by the repository owner. Local compilation, testing, and debugging are the next steps; no project should be marked Completed until it passes verification.

## Explore

- **[Open the project directory](./index.html)** — search projects and filter by language or difficulty.
- **[C projects](./C/README.md)** — 18 projects, source implementations added; local verification pending.
- **[C++ projects](./CPP/README.md)** — 18 projects with source implementations; local verification pending.
- **[Local verification tracker](./docs/LOCAL-VERIFICATION.md)** — compile/runtime evidence for all 36 projects; currently not tested.
- **[Contributing guide](./CONTRIBUTING.md)** — coding, documentation, and pull request expectations.
- **[Changelog](./CHANGELOG.md)** — notable repository updates.
- **[MIT License](./LICENSE)**

## Learning roadmap

Projects are grouped into three difficulty levels. Use the labels as a suggested learning path, not a guarantee that every project has the same scope.

| Language | Beginner | Intermediate | Advanced | Total |
| --- | ---: | ---: | ---: | ---: |
| C | 6 | 6 | 6 | 18 |
| C++ | 6 | 6 | 6 | 18 |
| **Total** | **12** | **12** | **12** | **36** |

### C projects

<details>
<summary><strong>Beginner (6)</strong></summary>

1. [Calculator](./C/Calculator/README.md)
2. [Number Guessing Game](./C/Number-Guessing-Game/README.md)
3. [Unit Converter](./C/Unit-Converter/README.md)
4. [Student Grade Calculator](./C/Student-Grade-Calculator/README.md)
5. [Simple Interest Calculator](./C/Simple-Interest-Calculator/README.md)
6. [Rock Paper Scissors](./C/Rock-Paper-Scissors/README.md)

</details>

<details>
<summary><strong>Intermediate (6)</strong></summary>

1. [Tic-Tac-Toe](./C/Tic-Tac-Toe/README.md)
2. [Student Management System](./C/Student-Management-System/README.md)
3. [Contact Book](./C/Contact-Book/README.md)
4. [Expense Tracker](./C/Expense-Tracker/README.md)
5. [Quiz Application](./C/Quiz-Application/README.md)
6. [Inventory Management System](./C/Inventory-Management-System/README.md)

</details>

<details>
<summary><strong>Advanced (6)</strong></summary>

1. [Phonebook Application](./C/Phonebook-Application/README.md)
2. [Basic HTTP Server](./C/Basic-HTTP-Server/README.md)
3. [Mini Shell](./C/Mini-Shell/README.md)
4. [File Compression Tool](./C/File-Compression-Tool/README.md)
5. [Terminal Text Editor](./C/Terminal-Text-Editor/README.md)
6. [Memory Allocator Simulator](./C/Memory-Allocator-Simulator/README.md)

</details>

### C++ projects

<details>
<summary><strong>Beginner (6)</strong></summary>

1. [Quiz Game](./CPP/Quiz-Game/README.md)
2. [Task Manager App](./CPP/Task-Manager-App/README.md)
3. [To-Do List](./CPP/To-Do-List/README.md)
4. [Password Generator](./CPP/Password-Generator/README.md)
5. [Digital Clock](./CPP/Digital-Clock/README.md)
6. [Bank Account Simulator](./CPP/Bank-Account-Simulator/README.md)

</details>

<details>
<summary><strong>Intermediate (6)</strong></summary>

1. [Library Management System](./CPP/Library-Management-System/README.md)
2. [Bank Management System](./CPP/Bank-Management-System/README.md)
3. [Snake Game](./CPP/Snake-Game/README.md)
4. [Employee Payroll System](./CPP/Employee-Payroll-System/README.md)
5. [Parking Lot Management System](./CPP/Parking-Lot-Management-System/README.md)
6. [Tic-Tac-Toe with AI](./CPP/Tic-Tac-Toe-with-AI/README.md)

</details>

<details>
<summary><strong>Advanced (6)</strong></summary>

1. [Custom String Class](./CPP/Custom-String-Class/README.md)
2. [Chess Game](./CPP/Chess-Game/README.md)
3. [Chat Application using Sockets](./CPP/Chat-Application-using-Sockets/README.md)
4. [Mini Database Engine](./CPP/Mini-Database-Engine/README.md)
5. [Memory Pool Allocator](./CPP/Memory-Pool-Allocator/README.md)
6. [Multithreaded Download Manager](./CPP/Multithreaded-Download-Manager/README.md)

</details>

## Repository structure

```text
C-CPP-Playground/
├── C/                       # C project folders and category README
├── CPP/                     # C++ project folders and category README
├── index.html                # Static project directory
├── README.md                 # This guide
└── LICENSE
```

Each project folder contains a README documenting its scope and status. All 18 C and all 18 C++ projects now have initial source implementations. See the [C project roadmap](./C/README.md) and [C++ project roadmap](./CPP/README.md) for each project's scope, build instructions, and verification checklist. We are implementing the full project collection first; local compilation, execution, and debugging will follow afterward.

## Compiling projects

The website itself uses **plain HTML, CSS, and vanilla JavaScript**. It has no package manager, framework, or build step and is designed to work with GitHub Pages.

The C/C++ projects are native programs, not programs that run directly inside the website. Use a compatible compiler such as GCC or Clang and follow the exact commands documented in each project README. The initial C implementations are in [`C/Calculator/main.c`](./C/Calculator/main.c), [`C/Number-Guessing-Game/main.c`](./C/Number-Guessing-Game/main.c), and [`C/Unit-Converter/main.c`](./C/Unit-Converter/main.c). Native programs will be compiled and run locally after the planned implementations have been added; GitHub Actions is not used to compile or execute these programs.

Typical compiler commands (examples only; source files must exist first):

```bash
# C example
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o app
./app

# C++ example
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o app
./app
```

Some advanced projects may need additional libraries, platform-specific APIs, or linker flags; their own README must document these requirements.

## Project status definitions

- **Planned** — folder and scope are documented; implementation is not yet available.
- **In Progress** — working source code exists, but implementation or validation is still underway.
- **Completed** — source code, usage/build documentation, and appropriate checks are in place.

The website catalogue must reflect actual repository contents. Never mark a project completed just because its folder or README exists.

## Contributing

Contributions and constructive suggestions are welcome.

1. Choose a project from the lists above and check its README for scope.
2. Keep each project self-contained in its existing folder.
3. Write readable, maintainable C or C++ and validate user input.
4. Document compiler requirements, build/run commands, features, usage, and known limitations.
5. Test normal inputs, invalid inputs, and relevant edge cases; report what you tested.
6. Keep the catalogue's project status and source link consistent with the actual files.
7. Avoid unrelated dependencies or generated build artifacts unless a project genuinely needs them.

For larger changes, open an issue or discussion first so the intended scope is clear.

## Learning goals

- Progress from language fundamentals to data structures, algorithms, file handling, networking, and memory management.
- Practise breaking requirements into small, testable programs.
- Build confidence with debugging, error handling, documentation, and compiler warnings.
- Keep the project catalogue accessible, responsive, and usable without a JavaScript framework.

## License

This repository is licensed under the [MIT License](./LICENSE). Check the license before reusing or redistributing any individual contribution.
