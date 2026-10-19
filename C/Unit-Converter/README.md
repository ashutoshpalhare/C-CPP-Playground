# Unit Converter

> **Status: In Progress** — the initial source implementation is available. Local compilation and conversion testing are intentionally deferred until the project collection is implemented.

## Overview

A menu-driven command-line unit converter written in standard C. Choose a conversion, enter a numeric value, and view the converted result.

## Features

- Converts kilometres and miles in both directions.
- Converts metres and feet in both directions.
- Converts kilograms and pounds in both directions.
- Converts Celsius and Fahrenheit in both directions.
- Repeats conversions until the user enters `q`.
- Validates menu choices and numeric input.
- Detects non-finite input and converted results.

## Concepts practised

- Functions and `switch` statements
- Menu-driven program flow
- Numeric parsing with `strtod`
- Input validation and error handling
- Floating-point arithmetic and conversion formulas

## Requirements

- A C compiler with C11 support (GCC or Clang recommended).
- A terminal.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o unit-converter -lm
./unit-converter
```

If you use Clang, replace `gcc` with `clang`.

## Conversion menu

| Option | Conversion |
| --- | --- |
| 1 | Kilometres to miles |
| 2 | Miles to kilometres |
| 3 | Metres to feet |
| 4 | Feet to metres |
| 5 | Kilograms to pounds |
| 6 | Pounds to kilograms |
| 7 | Celsius to Fahrenheit |
| 8 | Fahrenheit to Celsius |
| q | Quit |

## Example session

```text
Welcome to the C Unit Converter.

========== Unit Converter ==========
1. Kilometres to miles
2. Miles to kilometres
3. Metres to feet
4. Feet to metres
5. Kilograms to pounds
6. Pounds to kilograms
7. Celsius to Fahrenheit
8. Fahrenheit to Celsius
q. Quit
Choose a conversion (1-8, q to quit): 7
Enter the value to convert: 25
Converted value: 77 F
```

## Local verification checklist

- [ ] Compile with warnings enabled on GCC or Clang.
- [ ] Check each conversion against known values.
- [ ] Try zero, negative, decimal, and very large finite values.
- [ ] Try blank input, text, trailing characters, and invalid menu options.
- [ ] Verify quitting and end-of-file behaviour.
- [ ] Check handling of results outside the supported numeric range.

## Current limitations

- Uses common approximate conversion constants.
- Does not perform currency, volume, area, or time conversions.
- The source has not yet been compiled or run; local testing will happen after the planned projects are added.
