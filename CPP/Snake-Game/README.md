# Snake Game

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

The classic snake game on a 30×18 grid rendered in the terminal. Eat `*` to grow and score, avoid the walls and your tail; the game speeds up as you score. High score persists in `snake_highscore.txt`.

## Features

- Smooth redraw using ANSI cursor-home sequences (no flicker from full clears).
- WASD or arrow keys; direction reversal is ignored; pause (`P`), quit (`Q`), restart (`R`).
- Speed scales with score (180 ms → 60 ms per tick).
- Food never spawns on the snake; full board counts as a win.
- Persistent high score.
- RAII `Terminal` class restores console settings even on early exit.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- Must run in a real interactive terminal (see platform table).
- Linux: `-pthread` may be required for `std::this_thread`.

## Compile

```bash
# Linux / macOS / WSL
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o snake -pthread

# Windows (MinGW-w64)
g++ -std=c++17 -Wall -Wextra main.cpp -o snake.exe

# Windows (MSVC)
cl /std:c++17 /EHsc /W4 main.cpp /Fe:snake.exe
```

## Run

```bash
./snake       # or snake.exe
```

## Usage example

```text
 SNAKE   score: 30   best: 120   length: 6
+------------------------------+
|                              |
|        ooooo@                |
|                     *        |
|                              |
+------------------------------+
 WASD/arrows move  P pause  Q quit
```

## File storage

`snake_highscore.txt` in the working directory (a single integer).

## Platform compatibility

| Platform | Supported | Notes |
| --- | --- | --- |
| Windows 10 (1511+) / 11 | Yes | Uses `<conio.h>` for keys and VT sequences for drawing. |
| Linux / macOS / WSL | Yes | `termios` raw mode + `select()`; arrow keys decoded from escape sequences. |
| Older Windows consoles | Not supported | Frame redraw needs VT processing. |

## Known limitations

- Fixed board size; terminal must be at least 34 columns × 24 rows.
- No sound, colours or levels.
- Input is sampled once per tick; very fast double key presses may be collapsed.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Keyboard handling on Windows and Linux
- [ ] Wall and self collision
- [ ] Speed increase and high-score save
- [ ] Terminal state restored after quitting

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
