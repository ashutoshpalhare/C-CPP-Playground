# Library Management System

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A catalogue-and-loans system for a small library: books with multiple copies, members, issuing and returning with due dates, overdue detection and late fines. Three text files keep the data between runs.

## Features

- Add/remove/search books (title, author, ISBN) with copy counts.
- Register members; limit of 3 concurrent loans per member; members with overdue items cannot borrow.
- Issue and return with 14-day loan period and a $0.25/day late fine.
- Active and overdue loan reports with days late.
- `std::map` for id lookups, `std::optional` parsing helpers, lambda-based generic file loader.
- Malformed lines are reported and skipped, never crash.

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
Loaded 2 books, 1 members, 0 active loans.

=== Library Management ===
 1. List/search books      2. Add book          3. Remove book
 ...
Choose: 6
Book ID: 1
Member ID: 1
Issued. Due on 2025-03-18.
Choose: 8
Loan  Book                          Member              Due         Status
1     The C Programming Language    Dennis              2025-03-18  on time
```

## File storage

`books.txt` (`id|title|author|isbn|total|available`), `members.txt` (`id|name|email`) and `loans.txt` (`id|book|member|issuedDay|dueDay|returnedDay`, days since Unix epoch, `-1` = not returned). All files are in the working directory and saved on exit and on demand.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Dates are whole days computed from the system clock in UTC; no manual date entry.
- Fields may not contain `|`.
- No reservations/holds, no renewals, single-user access.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Issue limits and overdue blocking
- [ ] Fine calculation after simulated late return (edit loans.txt dueDay)
- [ ] Removing a book with copies on loan is refused
- [ ] Corrupt data lines are skipped

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
