# Digital Clock

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A live terminal clock with large ASCII digits, 12/24-hour toggle, a stopwatch and a countdown timer. The display refreshes in place once per second using ANSI escape sequences while keys are polled without blocking.

## Features

- Live clock with big 3-row digits and full date.
- `h` toggles 12/24-hour format; `q` returns to the menu.
- Stopwatch with start/pause/reset (100 ms resolution display).
- Countdown timer (1 s – 24 h) with terminal bell on completion.
- Non-blocking key polling on Windows (`_kbhit`) and POSIX (`select` + raw termios).
- Falls back to scrolling output on consoles without VT support.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- Interactive terminal (see platform table). No external libraries.
- Linux: add `-pthread` if your toolchain requires it for `std::this_thread`.

## Compile

```bash
# Linux / macOS / WSL
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o clock -pthread

# Windows (MinGW-w64)
g++ -std=c++17 -Wall -Wextra main.cpp -o clock.exe

# Windows (MSVC)
cl /std:c++17 /EHsc /W4 main.cpp /Fe:clock.exe
```

## Run

```bash
./clock       # or clock.exe
```

## Usage example

```text
=== Digital Clock ===

 _   _       _   _       _   _
| | |_|  .   _|  _|  .  |_  |_|
|_|  _|  .  |_   _|  .   _| |_|

Tuesday, 4 March 2025
[h] toggle 12/24h   [q] back to menu
```

## File storage

None.

## Platform compatibility

| Platform | Supported | Notes |
| --- | --- | --- |
| Windows 10 (1511+) / 11 | Yes | cmd.exe, PowerShell, Windows Terminal. VT mode enabled automatically. |
| Older Windows consoles | Partial | Output scrolls instead of redrawing in place. |
| Linux / macOS / WSL | Yes | Any ANSI terminal; uses `termios`. |
| IDE output panes | No | Not real terminals; run from a terminal window. |

## Known limitations

- Time zone comes from the C runtime's local time; no manual zone selection.
- Clock accuracy depends on `sleep_for` granularity (±100 ms visual jitter at worst).
- Terminal must be at least ~40 columns wide for the big digits to render properly.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Clock redraw and 12/24 toggle on Windows and Linux
- [ ] Stopwatch pause/resume accuracy
- [ ] Countdown completion and cancel
- [ ] Behaviour when terminal is resized

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
