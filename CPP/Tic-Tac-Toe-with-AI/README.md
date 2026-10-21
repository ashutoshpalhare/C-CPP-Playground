# Tic-Tac-Toe with AI

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

Play tic-tac-toe against a computer opponent powered by minimax with alpha-beta pruning. Hard mode is unbeatable; Medium mixes in random moves; Easy is random. Two-player mode and a running scoreboard are included.

## Features

- Minimax with alpha-beta pruning and depth-aware scoring (prefers quicker wins, slower losses).
- Random tie-breaking among equally good moves so games vary.
- Three difficulty levels; choose to play first (X) or second (O).
- Two-player hot-seat mode.
- Input validation for squares 1–9 and occupied cells; EOF-safe.
- Board, AI and I/O separated into small classes/functions.

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
=== Tic-Tac-Toe with AI ===
Mode: (1) vs computer  (2) two players  (q) quit: 1
Difficulty: (e)asy  (m)edium  (h)ard/unbeatable: h
Play as (x) first or (o) second: x

 1 | 2 | 3
---+---+---
 4 | 5 | 6
---+---+---
 7 | 8 | 9

Player X, choose a square (1-9): 5
Computer (O) plays 1.
...
It's a draw.
Scoreboard - X: 0  O: 0  Draws: 1
```

## File storage

None.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- 3×3 board only.
- Minimax explores the full tree each move (fast enough for 3×3, not generalised).
- No undo.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Hard AI never loses across many games
- [ ] Easy/medium behave differently
- [ ] Occupied square and invalid input handling
- [ ] Two-player mode and scoreboard

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
