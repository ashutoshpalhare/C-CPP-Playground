# Quiz Application

> **Status: In Progress** — initial C source and usage documentation are available. Compilation and functional testing are deferred until the planned project implementations are added.

## Overview

A terminal-based multiple-choice quiz about C fundamentals. Answer ten questions, receive immediate correctness feedback, and review the score and explanations at the end.

## Features

- Ten built-in multiple-choice questions with four options each.
- Case-insensitive answers (A–D).
- Input validation and graceful handling of closed input.
- Immediate feedback for each answer.
- Final score, percentage, feedback message, and answer explanations.
- Questions and options stored in a `struct` array.

## Requirements

- C11-compatible compiler such as GCC or Clang.
- Terminal for interactive input.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o quiz-app
./quiz-app
```

On Windows, run `quiz-app.exe`.

## Example

```text
Question 1/10: Which header declares printf()?
  A) <stdio.h>
  B) <stdlib.h>
  C) <string.h>
  D) <ctype.h>
Your answer: A
Correct!
...
Score: 8/10 (80.0%)
```

## Concepts practised

- Arrays of structures
- Loops and conditional logic
- String handling and input validation
- Score calculation and formatted output
- Organising question data separately from quiz flow

## Limitations

- Questions are built into the source; there is no external question file or editor.
- Each question has one correct answer and four fixed options.
- The quiz does not save player scores between runs.

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Test correct and incorrect answers.
- [ ] Test lowercase answers and invalid input.
- [ ] Verify score, percentage, and answer review.
- [ ] Test EOF/closed input and overlong answers.

**Verification note:** The source has not yet been compiled or run. Local verification is deferred until the initial project implementations are added across the collection. GitHub Actions is not used to compile or execute native C programs.
