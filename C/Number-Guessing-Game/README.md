# Number Guessing Game

> **Status: In Progress** — the first source implementation is available. Local compilation and gameplay testing are intentionally deferred until the project collection is implemented.

## Overview

A command-line number guessing game written in standard C. The program chooses a secret number from 1 to 100 and guides the player with higher/lower hints until the correct number is guessed or the player quits.

## Features

- Chooses a pseudo-random number between 1 and 100.
- Gives higher/lower hints after each valid guess.
- Counts valid guesses and reports the attempt total when solved.
- Rejects non-numeric, out-of-range, and malformed input.
- Accepts `q` to leave the round.
- Uses standard C library facilities only.

## Concepts practised

- Pseudo-random numbers with `rand` and `srand`
- Loops and conditional logic
- Reading and validating terminal input
- Parsing integers with `strtol`
- Functions, enums, and return values

## Requirements

- A C compiler with C11 support (GCC or Clang recommended).
- A terminal.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o number-guessing-game
./number-guessing-game
```

If you use Clang, replace `gcc` with `clang`.

## Example session

The secret number and hints vary between runs.

```text
================================
        Number Guessing Game
================================
Guess the secret number from 1 to 100.
Enter q to quit the current round.

I picked a number between 1 and 100. Can you guess it?
Your guess (1-100, or q to quit): 50
Too low. Try a higher number.
Your guess (1-100, or q to quit): 75
Too high. Try a lower number.
Your guess (1-100, or q to quit): q

Round ended.
Thanks for playing!
```

## Local verification checklist

- [ ] Compile with warnings enabled on GCC or Clang.
- [ ] Verify guesses that are too low, too high, and correct.
- [ ] Verify the attempt count only includes valid guesses.
- [ ] Try blank input, text, decimals, trailing characters, and values outside 1–100.
- [ ] Verify quitting and end-of-file behaviour.
- [ ] Play multiple runs and confirm the secret number can vary.

## Current limitations

- One round is played per program launch.
- The number generator is intended for a learning game, not security-sensitive randomness.
- The source has not yet been compiled or run; local testing will happen after the planned projects are added.
