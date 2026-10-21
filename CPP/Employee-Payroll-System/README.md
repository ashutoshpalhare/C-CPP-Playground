# Employee Payroll System

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

Computes monthly payroll for salaried, hourly (with overtime) and commission-based employees using a polymorphic class hierarchy, applies a progressive tax table, prints payslips and department reports, and persists employee records.

## Features

- Abstract `Employee` with `SalariedEmployee`, `HourlyEmployee` (1.5× overtime above 160 h) and `CommissionEmployee`.
- Progressive tax bands (0 / 10 / 20 / 30 %).
- Payslip per employee, payroll table with totals, department totals, sort by net pay.
- Update monthly hours/sales via `dynamic_cast`.
- Factory function parses the data file; `std::unique_ptr` owns employees.
- Integer-cent money, validated numeric input, `|` rejected in text fields.

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
=== Employee Payroll System ===
Choose: 2
Type (s=salaried, h=hourly, c=commission): h
Name: Priya
Department: Support
Hourly rate: 20
Hours this month: 170
Added employee #1.
Choose: 3
Employee ID: 1

------------ PAYSLIP ------------
Employee : Priya (#1)
Basis    : 170.0 h @ 20.00/h (incl. 10.0 h overtime at 1.5x)
Gross    :        3500.00
Tax      :         300.00
Net pay  :        3200.00
```

## File storage

`employees.txt` in the working directory, one employee per line: `id|name|dept|S|annualCents`, `...|H|rateCents|hours`, or `...|C|baseCents|salesCents|pct`.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Tax table and overtime threshold are constants in the source.
- No deductions beyond tax (pension, insurance, etc.).
- No pay history; the file stores only the current month's variables.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Each employee type's gross/tax/net
- [ ] Overtime boundary at exactly 160 h
- [ ] Tax band boundaries (1000, 3000, 6000)
- [ ] Reload after save produces identical table

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
