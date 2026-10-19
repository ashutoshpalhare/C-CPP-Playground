# Student Grade Calculator

> **Status: In Progress** — initial source is available. Compilation, execution, and debugging are deferred until the project collection is implemented.

## Overview

A command-line program that collects marks for a chosen number of subjects and reports total marks, average percentage, a letter grade, and pass/fail status.

## Features

- Accepts 1–100 subjects.
- Validates subject count and marks (whole numbers from 0 to 100).
- Handles malformed input, overlong lines, end-of-input, and allocation failure.
- Calculates total and average, then displays a letter grade and pass/fail result.
- Uses a documented, project-defined grade scale.

## Grade scale

| Average | Grade |
| --- | --- |
| 90–100 | A |
| 80–89.99… | B |
| 70–79.99… | C |
| 60–69.99… | D |
| 40–59.99… | E |
| Below 40 | F |

The program reports **PASS** for an average of 40 or above, otherwise **FAIL**. This example scale is for learning and is not an official institution grading policy.

## Concepts to practise

- Dynamic memory allocation with `calloc` and `free`
- Functions, arrays, and conditionals
- Input parsing and validation
- Arithmetic and formatted output

## Build and run

Requires a C11-compatible compiler such as GCC or Clang.

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o grade-calculator
./grade-calculator
```

On Windows, run `grade-calculator.exe` after compiling with your installed compiler.

## Example

```text
Number of subjects (1-100): 3
Marks for subject 1 (0-100): 85
Marks for subject 2 (0-100): 90
Marks for subject 3 (0-100): 75

------------ RESULT ------------
Subjects       : 3
Total marks    : 250 / 300
Average        : 83.33%
Grade          : B
Result         : PASS
--------------------------------
```

## Local verification checklist

- [ ] Compile with the command above and review compiler warnings.
- [ ] Check 1 subject and the maximum 100 subjects.
- [ ] Check marks 0, 40, 60, 70, 80, 90, and 100.
- [ ] Check negative marks, marks above 100, decimals, letters, and blank input.
- [ ] Check end-of-input while entering the subject count and marks.

**Verification note:** Source and documentation have been added, but the program has not yet been compiled or run. Testing will happen locally after the planned implementations are in place. No GitHub Actions workflow is used to compile or execute this native program.
