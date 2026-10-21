# Quiz Game

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A replayable terminal quiz with three categories (C++ Basics, General Knowledge, Mathematics) or an all-categories round. Questions are shuffled each game, answers are validated, and a review of missed questions with explanations is shown at the end.

## Features

- Category selection or combined round.
- Questions shuffled with `std::mt19937`.
- Letter answers A–D, case-insensitive, with re-prompting on bad input.
- Immediate feedback and end-of-game summary with percentage and review.
- Replay loop; graceful handling of closed input (EOF).
- `QuestionBank` and `Quiz` classes keep data separate from game flow.

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
=== Quiz Game ===

Categories:
  1) C++ Basics
  2) General Knowledge
  3) Mathematics
  4) All categories
Choice (1-4): 1

Question 1 of 5: Which header provides std::cout?
  A) <stdio.h>
  B) <iostream>
  C) <stream>
  D) <cstdout>
Your answer (A-D): b
Correct!
...
=== Results ===
Score: 4/5 (80%)
Great job!
```

## File storage

None — questions are built into the source; no files are read or written.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Questions are hard-coded; there is no external question file.
- No timer or per-question scoring weights.
- Scores are not saved between runs.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Each category and the combined round
- [ ] Invalid answers (empty, two letters, digits)
- [ ] EOF handling mid-quiz
- [ ] Replay loop

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
