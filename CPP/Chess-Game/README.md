# Chess Game

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A complete two-player terminal chess implementation with full rule enforcement: every move is validated by generating pseudo-legal moves and filtering out those that leave the king in check. Special rules (castling, en passant, promotion), check/checkmate/stalemate, the 50-move rule and basic insufficient-material draws are handled. An optional simple computer opponent is included.

## Features

- All piece moves, castling (with through-check tests), en passant, promotion with piece choice (`e7e8q`, defaults to queen).
- Check, checkmate, stalemate, 50-move rule, insufficient material (K, K+N, K+B).
- Coordinate-notation input (`e2e4`), `moves` to list legal moves, `undo`, `history`, `resign`.
- Optional computer opponent (one-ply material search with a one-move look-ahead penalty and random tie-breaking) — beatable by design.
- Board snapshots give a simple, correct undo.
- ASCII board (uppercase = White, lowercase = Black) that renders in any console.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- No external libraries.

## Compile

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o chess
```

## Run

```bash
./chess        # or chess.exe
```

## Usage example

```text
=== Chess ===
Play against the computer? (y/N): n

    a b c d e f g h
   -----------------
 8 | r n b q k b n r | 8
 7 | p p p p p p p p | 7
 6 | . . . . . . . . | 6
 5 | . . . . . . . . | 5
 4 | . . . . . . . . | 4
 3 | . . . . . . . . | 3
 2 | P P P P P P P P | 2
 1 | R N B Q K B N R | 1
   -----------------
    a b c d e f g h

1. White to move > e2e4
1. Black to move > e7e5
2. White to move > moves
29 legal move(s): a2a3 a2a4 b2b3 ... g1f3 g1h3 ...
```

## File storage

None (no PGN save/load).

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- No threefold-repetition detection.
- The computer opponent is intentionally weak (no deep search).
- Input is coordinate notation only; standard algebraic notation (`Nf3`) is not parsed.
- No clocks.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Castling legality incl. moving through check
- [ ] En passant only on the immediately following move
- [ ] Promotion to each piece
- [ ] Fool's mate (f2f3 e7e5 g2g4 d8h4) ends in checkmate
- [ ] Stalemate and 50-move rule positions
- [ ] Undo restores castling rights and en passant square

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
