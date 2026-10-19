# Calculator

> **Status: In Progress** — the first implementation is available; compiler and edge-case verification are still to be completed.

## Overview

A small, interactive command-line calculator written in standard C. It supports addition, subtraction, multiplication, and division, with input validation and clear error messages.

## Features

- Supports `+`, `-`, `*`, and `/`.
- Accepts decimal and negative numbers.
- Rejects malformed, non-finite, and out-of-range numeric input.
- Handles unsupported operators and division by zero.
- Repeats calculations until the user enters `q`.
- Detects results outside the supported floating-point range.

## Concepts practised

- Functions and `switch` statements
- Character and string handling
- Parsing numeric input with `strtod`
- Input validation and error handling
- Floating-point arithmetic

## Requirements

- A C compiler with C11 support (GCC or Clang recommended).
- A terminal.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o calculator -lm
./calculator
```

If you use Clang, replace `gcc` with `clang`. The `-lm` flag links the math library on toolchains that require it.

## Example

```text
================================
       C Command-Line Calculator
================================
Supported operations: +  -  *  /
Enter q at the operator prompt to quit.

Operator (+, -, *, /, q): *
First number: 6
Second number: 7
Result: 42

Operator (+, -, *, /, q): /
First number: 8
Second number: 0
Error: division by zero is not allowed.

Operator (+, -, *, /, q): q
Calculator closed. Happy coding!
```

## Test checklist

- [ ] Addition, subtraction, multiplication, and division
- [ ] Decimal and negative operands
- [ ] Invalid numeric input and trailing characters
- [ ] Unsupported operator
- [ ] Division by zero
- [ ] Empty input and end-of-file
- [ ] Very large numeric input and non-finite results
- [ ] Compile with warnings enabled on GCC or Clang

## Current limitations

- This is a terminal calculator; it has no graphical interface or calculation history.
- It uses `double` floating-point arithmetic, so results are not intended for exact financial calculations.
- The test checklist remains open until the program is compiled and exercised in a supported environment.
