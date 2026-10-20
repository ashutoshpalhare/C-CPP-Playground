# Expense Tracker

> **Status: In Progress** — initial C source and usage documentation are available. Compilation and functional testing are deferred until the planned project implementations are added.

## Overview

A menu-driven terminal app for recording everyday expenses, listing saved transactions, and calculating a basic spending summary. Records are saved locally in `expenses.csv` and remain available after restarting the program.

## Features

- Add expenses with an automatically assigned ID, date, category, description, and amount.
- List saved expenses with a running total.
- View count, total spending, and average expense.
- Validate date format, required text fields, menu choices, and positive finite amounts.
- Warn when malformed records are encountered.
- Handle missing files and common file-operation errors.

## Requirements

- C11-compatible compiler such as GCC or Clang.
- Terminal with write permission in the working directory.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o expense-tracker
./expense-tracker
```

On Windows, run `expense-tracker.exe`.

## Example session

```text
1. Add expense
2. List expenses
3. View summary
4. Exit
Choose an option: 1
New expense ID: 1
Date (YYYY-MM-DD): 2026-10-20
Category (e.g. Food, Travel): Food
Description: Lunch
Amount (positive number): 180
Expense saved successfully.
```

## Data storage

The app creates `expenses.csv` in the current working directory. Each record follows this format:

```text
ID,YYYY-MM-DD,Category,Description,Amount
```

This is a deliberately simple comma-separated format, not a full CSV implementation: category and description fields cannot contain commas. Amounts are saved to two decimal places. The date check validates the `YYYY-MM-DD` shape and basic month/day ranges; it does not validate month-specific day counts or leap years. Concurrent program instances are not supported.

## Concepts practised

- Structures and functions
- File input/output and record parsing
- Numeric conversion and validation
- Aggregation (count, total, average)
- Menu-driven program design

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Add expenses and verify they persist after restarting.
- [ ] List records and verify totals.
- [ ] Confirm summary count, total, and average.
- [ ] Test invalid dates, malformed amounts, empty fields, long fields, and missing data file.

**Verification note:** The source has not yet been compiled or run. Local verification is deferred until the initial project implementations are added across the collection. GitHub Actions is not used to compile or execute native C programs.
