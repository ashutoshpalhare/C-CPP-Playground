# Rock Paper Scissors

> **Status: In Progress** — initial C source and usage documentation are available. Local compilation and gameplay testing are intentionally deferred until the planned project implementations are in place.

## Overview

Play Rock Paper Scissors against the computer in a terminal. The game chooses randomly, announces each round's result, and tracks your wins, computer wins, and ties until you quit.

## Features

- Accepts `rock`, `paper`, `scissors`, their first letters, or menu values `1`, `2`, `3` (case-insensitive).
- Generates a computer choice using the C standard library pseudo-random number generator.
- Explains each round's outcome and keeps a running score.
- Prints a final score and overall comparison when you quit.
- Handles invalid choices, overlong input, and end-of-input gracefully.

## Rules

- Rock beats scissors.
- Scissors beats paper.
- Paper beats rock.
- Matching choices are a tie.

## Requirements

- A C compiler supporting C11, such as GCC or Clang.
- A terminal.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o rock-paper-scissors
./rock-paper-scissors
```

With Clang, replace `gcc` with `clang`. On Windows, run the generated executable as `rock-paper-scissors.exe`.

## Example session

```text
================================
       ROCK PAPER SCISSORS
================================
Choose 1/Rock, 2/Paper, or 3/Scissors.
Enter q at the prompt to quit.

Your choice [1/2/3 or r/p/s]: rock
You chose: Rock
Computer chose: Scissors
You win this round!
Score — You: 1 | Computer: 0 | Ties: 0

Your choice [1/2/3 or r/p/s]: q

========== FINAL SCORE ==========
You: 1 | Computer: 0 | Ties: 0
Overall result: You are ahead!
Thanks for playing!
```

The computer choice and score above are illustrative; actual results vary.

## Concepts practised

- Enums and functions
- Input parsing and validation
- Loops and conditional logic
- Pseudo-random number generation
- Score tracking and formatted output

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Try each choice using words, letters, and numeric options.
- [ ] Check all win/loss/tie combinations.
- [ ] Try invalid and overlong input.
- [ ] Quit with `q` and test end-of-input behaviour.

**Verification note:** Source code has been added, but it has not yet been compiled or run. Local testing will happen after the collection's planned implementations are added. Native C programs are not compiled or executed by GitHub Actions.
