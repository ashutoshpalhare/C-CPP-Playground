# To-Do List

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A minimal command-line to-do list driven by short commands (`add`, `done`, `undo`, `edit`, `remove`, `clear`, `list`). Every change is saved immediately to `todo.txt`.

## Features

- Numbered items with `[ ]`/`[x]` state.
- Add, complete, reopen, edit, remove and bulk-clear completed items.
- Input parsed with `std::istringstream`; item numbers validated against the current list size.
- Auto-save after every change and on exit.
- Human-readable data file you can edit by hand.

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
To-Do List (0 item(s) loaded from todo.txt). Type 'help' for commands.
todo> add Buy milk
Added item 1.
todo> add Write README
Added item 2.
todo> done 1
Completed: Buy milk
todo> list
1. [x] Buy milk
2. [ ] Write README
2 item(s), 1 pending.
todo> clear
Removed 1 completed item(s).
todo> quit
Saved 1 item(s) to todo.txt. Bye!
```

## File storage

`todo.txt` in the working directory. Each line is `[ ] text` or `[x] text`. Malformed lines are ignored on load.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- No priorities, dates or categories (see Task Manager App for those).
- No undo for `remove`/`clear`.
- Not designed for concurrent use from multiple processes.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] All commands including invalid indices
- [ ] Empty `add`
- [ ] Hand-edited todo.txt with a malformed line
- [ ] EOF (Ctrl+D / Ctrl+Z) exits cleanly

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
