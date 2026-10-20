# Inventory Management System

> **Status: In Progress** — initial C source and usage documentation are available. Compilation and functional testing are deferred until the planned project implementations are added.

## Overview

A menu-driven terminal inventory tool for maintaining a small local list of items. Add items, browse stock, update quantities, and search by item name or category. Records persist in `inventory.csv`.

## Features

- Add items with an automatically assigned ID, name, category, quantity, and unit price.
- List inventory with per-item stock value and an estimated total stock value.
- Update an item's quantity by ID, including setting it to zero.
- Search by item name or category.
- Validate required text fields, menu options, quantities, and prices.
- Handle missing files and common file-operation errors.

## Requirements

- C11-compatible compiler such as GCC or Clang.
- Terminal with write permission in the working directory.
- The math library is used for finite-number validation; GCC/Clang builds may use `-lm`.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o inventory
./inventory
```

If your toolchain requires explicit math-library linking:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o inventory -lm
```

On Windows, run `inventory.exe`.

## Example session

```text
1. Add item
2. List inventory
3. Update item quantity
4. Search items
5. Exit
Choose an option: 1
New item ID: 1
Item name: USB Keyboard
Category: Accessories
Quantity (whole number, zero or more): 12
Unit price (zero or more): 799
Item added successfully.
```

## Data storage

The app creates `inventory.csv` in the current working directory. Each record follows this format:

```text
ID,Name,Category,Quantity,UnitPrice
```

This is a simple comma-separated format, not a full CSV implementation. Names and categories cannot contain commas. Unit prices are saved to two decimal places. Concurrent program instances are not supported.

## Concepts practised

- Structures and functions
- File input/output and record parsing
- Searching and updating records
- Numeric validation and stock-value aggregation
- Menu-driven program design

## Limitations

- Search is case-sensitive.
- There is no delete-item or purchase/sales history feature yet.
- The estimated stock value is quantity multiplied by unit price; it is not an accounting valuation.
- Quantity updates rewrite the data file through a temporary file. Keep a backup of important inventory records, especially before experimenting with the program.

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Add items and verify persistence after restarting.
- [ ] List items and verify per-item and total stock values.
- [ ] Update quantities, including zero, and confirm the correct item changes.
- [ ] Search by name/category.
- [ ] Test invalid IDs, negative quantities/prices, empty fields, long fields, and missing files.

**Verification note:** The source has not yet been compiled or run. Local verification is deferred until the initial project implementations are added across the collection. GitHub Actions is not used to compile or execute native C programs.
