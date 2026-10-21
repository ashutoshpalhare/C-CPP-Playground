# Terminal Text Editor

> **Status: In Progress** — C source and documentation are available. The code has **not** yet been compiled, run, or tested.

## Platform requirement: POSIX terminals only

The editor uses `termios` raw mode, `ioctl(TIOCGWINSZ)`, and ANSI/VT100 escape sequences. It is intended for **Linux, macOS, and Windows through WSL** in a VT100-compatible terminal (for example GNOME Terminal, iTerm2, or Windows Terminal running a WSL shell). It **does not build natively on Windows** (MinGW or MSVC) and is not meant for the legacy Windows console. It must be run in an interactive terminal, not with redirected input.

On Windows install WSL (`wsl --install`), then `sudo apt update && sudo apt install build-essential` inside it.

## Overview

A compact full-screen text editor that teaches how terminal programs work: switching the terminal to raw mode, decoding key presses, keeping text in memory, and redrawing the screen.

## Features

- Open an existing file or start a new one: `./mini-editor [file]`.
- Arrow keys, Home, End, Page Up, Page Down; Delete and Backspace; Enter splits lines, Backspace at line start joins them.
- Vertical and horizontal scrolling, status bar (name, line count, modified flag, cursor position), and temporary message bar.
- `Ctrl+S` saves (asks for a file name for new buffers); `Ctrl+Q` quits, requiring a second press if there are unsaved changes.
- Saves through a temporary file and a rename, so a failed write does not destroy the original.
- Restores the terminal on exit.

## Build and run

```bash
cd C/Terminal-Text-Editor
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o mini-editor
./mini-editor notes.txt
```

If a strict `-std=c11` build reports missing POSIX declarations on your system, try `-std=gnu11` and note it as an issue.

## Keys

| Key | Action |
| --- | --- |
| Arrows, Home, End, PgUp, PgDn | Move |
| Printable ASCII, Tab (inserts 4 spaces), Enter | Insert |
| Backspace, Delete | Remove text |
| Ctrl+S | Save |
| Ctrl+Q | Quit (press twice with unsaved changes) |

## File storage

The file is read when started and written only when you save. Saves write `<name>.tmp` and rename it over the file. Every line is written with a `\n` ending, so **CRLF files are converted to LF and a missing final newline is added**.

## Concepts practised

- Raw terminal mode with `termios`, and restoring it with `atexit`
- Parsing escape sequences for special keys
- Dynamic arrays of lines and in-place text editing
- Viewport scrolling and building a frame in a buffer before one `write`
- Safe file saving

## Known limitations

- ASCII only: non-ASCII bytes in files are shown as `?` (and preserved on save), and non-ASCII typing is ignored.
- Tab characters already in a file are shown as `>` (one column) and kept as tabs.
- Lines are not wrapped; long lines scroll horizontally.
- No undo/redo, search, selection, copy/paste, syntax highlighting, or line numbers.
- If the process is killed by a signal (for example `kill`), the terminal may stay in raw mode; type `reset` to fix it. `Ctrl+C` and `Ctrl+Z` are not treated as signals while the editor runs.
- A rename-based save replaces symbolic links and resets file permissions to the default.
- Holds the whole file in memory.

## Testing status

- [ ] Compiles on Linux or WSL with warnings enabled.
- [ ] Editing, line splitting/joining, scrolling, and resizing work.
- [ ] Save, save-as, and quit-with-unsaved-changes work; the terminal is restored on exit.
- [ ] Empty file, long lines, missing file, unreadable file, and read-only directory handled.

**Verification note:** this source has been reviewed by reading only. It has not been compiled or executed. GitHub Actions is not used to build or run native projects.
