# Task Manager App

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A menu-driven task organiser with priorities (Low/Medium/High), optional ISO due dates, completion state, editing and several sort orders. Tasks persist in `tasks.txt`.

## Features

- Add, edit, delete, complete/reopen tasks.
- Priority levels and validated `YYYY-MM-DD` due dates (leap years handled).
- List all or only pending tasks.
- Stable sorting by priority, due date, status or ID using lambdas.
- Automatic save on exit when there are unsaved changes.
- Malformed lines in the data file are skipped with a warning.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- No external libraries.

## Compile

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o app
```

## Run

```bash
./app        # Linux / macOS / WSL
app.exe      # Windows
```

## Usage example

```text
=== Task Manager ===
1. List all tasks
...
Choose: 3
Title: Finish C++ payroll project
Priority (1=Low, 2=Medium, 3=High) [2]: 3
Due date YYYY-MM-DD (optional): 2025-03-15
Added task #1.
Choose: 1
ID  Done  Prio    Due         Title
------------------------------------------------------------
1   [ ]   High    2025-03-15  Finish C++ payroll project
1 task(s), 1 pending.
```

## File storage

`tasks.txt` in the working directory, one task per line: `id|done|priority|due|title`. The first line is a `#` comment header.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Titles may contain `|` only as the last field (it is preserved) but the other fields may not.
- No recurring tasks, tags or reminders.
- Single user, single file.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Add/edit/delete/complete flows
- [ ] Invalid dates such as 2025-02-30
- [ ] Sorting by each key
- [ ] Corrupt line in tasks.txt is skipped

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
