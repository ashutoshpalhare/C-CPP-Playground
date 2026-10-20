# Tic Tac Toe

> **Status: In Progress** — initial C source and usage documentation are available. Compilation and gameplay testing are deferred until the planned project implementations are in place.

## Overview

A two-player terminal version of Tic Tac Toe. Players take turns placing X and O on a numbered 3×3 board. The game detects wins across rows, columns, and diagonals, and reports a draw if all cells are filled.

## Features

- Clear numbered 3×3 board that updates after each valid move.
- Alternating turns for Player X and Player O.
- Win detection for all rows, columns, and diagonals.
- Draw detection after nine valid moves.
- Input validation for non-numeric values, out-of-range positions, and occupied cells.
- Graceful handling of end-of-input.

## Requirements

- A C compiler supporting C11, such as GCC or Clang.
- A terminal.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o tic-tac-toe
./tic-tac-toe
```

With Clang, replace `gcc` with `clang`. On Windows, run `tic-tac-toe.exe`.

## How to play

1. Player X starts; Player O goes next.
2. Choose a free cell by entering its displayed number from 1 to 9.
3. The game rejects invalid or already occupied cells without consuming a turn.
4. Get three matching marks in a row, column, or diagonal to win.
5. If all nine cells are filled and nobody has won, the game ends in a draw.

## Example board

```text
 1 | 2 | 3
---+---+---
 4 | 5 | 6
---+---+---
 7 | 8 | 9
```

## Concepts practised

- One-dimensional arrays as a board representation
- Functions and constant data
- Input parsing and validation
- Loops, turn management, and conditionals
- Win and draw detection

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Test wins across all rows, columns, and both diagonals.
- [ ] Test a full-board draw.
- [ ] Try occupied cells, letters, blank input, and values outside 1–9.
- [ ] Test end-of-input during a turn.

**Verification note:** Source code has been added but has not yet been compiled or run. Local testing is planned after the collection's initial implementations are in place. Native C programs are not compiled or executed by GitHub Actions.
