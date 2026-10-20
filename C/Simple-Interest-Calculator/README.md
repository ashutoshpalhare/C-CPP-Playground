# Simple Interest Calculator

> **Status: In Progress** — initial C source is available. Compilation, execution, and debugging are deferred until the project collection is implemented.

## Overview

A command-line calculator for simple interest and the final amount after a given time period.

## Formula

- **Simple Interest (SI)** = (Principal × Annual Rate × Time) / 100
- **Total Amount** = Principal + SI

The rate is entered as an annual percentage and time in years. The program does not calculate compound interest.

## Features

- Accepts decimal values for principal, annual rate, and time.
- Rejects negative values, blank input, malformed numbers, non-finite values, and overlong input lines.
- Detects results too large to represent and handles end-of-input gracefully.
- Displays inputs, simple interest, and final amount to two decimal places.

## Concepts to practise

- Variables and arithmetic formulas
- Functions and input validation
- Floating-point numbers and formatted output
- Error handling with `strtod`, `errno`, and `isfinite`

## Build and run

Requires a C99-or-later compiler such as GCC or Clang. The `-lm` linker flag is used for the math library on systems that require it.

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o simple-interest -lm
./simple-interest
```

On Windows, run `simple-interest.exe` after compiling with your installed compiler.

## Example

```text
Principal amount: 10000
Annual interest rate (%): 5
Time period (years): 2

----------- RESULT -----------
Principal amount : 10000.00
Annual rate      : 5.00%
Time period      : 2.00 years
Simple interest  : 1000.00
Total amount     : 11000.00
------------------------------
```

## Local verification checklist

- [ ] Compile with the command above and review compiler warnings.
- [ ] Try zero values and decimal inputs.
- [ ] Try negative values, letters, blank input, and trailing characters.
- [ ] Try very large inputs and confirm overflow is reported safely.
- [ ] Test end-of-input at each prompt.

**Verification note:** The source and documentation have been added, but the program has not yet been compiled or run. Testing will happen locally after the planned project implementations are in place. No GitHub Actions workflow is used to compile or execute this native program.
