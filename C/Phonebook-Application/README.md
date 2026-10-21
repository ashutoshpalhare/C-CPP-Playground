# Phonebook Application

> **Status: In Progress** — C source and documentation are available. The code has **not** yet been compiled, run, or tested.

## Overview

A menu-driven terminal phonebook built around a **dynamically-sized array that is always kept sorted by name**. Keeping the data sorted lets the program use binary search for lookups and prefix searches, instead of scanning every entry.

## Features

- Add, edit, delete, and list contacts (name, phone, optional email).
- Names are unique, compared **case-insensitively**.
- Exact lookup by name using binary search.
- "Names starting with…" search that uses the sorted order.
- Free-text search across name, phone, and email (case-insensitive).
- List sorted by name or by phone number (`qsort` on a temporary copy).
- Edit supports renaming, keeping current values (leave blank), and clearing an email (`-`).
- Input validation: required fields, length limits, phone format (5–15 digits), simple email check, no control characters.
- Safe saving through a temporary file; unsaved-changes prompt on quit.

## Requirements

- C11 compiler (GCC or Clang; MinGW-w64 on Windows works).
- Write permission in the working directory.

## Build and run

```bash
cd C/Phonebook-Application
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o phonebook
./phonebook          # Windows: phonebook.exe
```

## Data storage

Saved to `phonebook.tsv` in the **current working directory** as tab-separated text:

```text
PHONEBOOK-V1
Asha Patil<TAB>9876543210<TAB>asha@example.com
Ravi Kumar<TAB>+91 98765 43210<TAB>
```

- The first line must be the header `PHONEBOOK-V1`; otherwise the file is not loaded.
- Malformed or duplicate lines are skipped and counted in a warning on startup.
- Saving writes `phonebook.tsv.tmp` first, then replaces the data file. On Windows the old file is removed before renaming, so a crash in that tiny window could leave only the `.tmp` file (the program tells you if this happens).
- Keep a backup before editing the file by hand. Multiple running copies are not supported.

## Example session

```text
=== Phonebook ===
1. Add contact
...
Choose an option: 1
Name: Asha Patil
Phone: 9876543210
Email (optional): asha@example.com
Contact added.
```

## Concepts practised

- `realloc` growth strategy and `memmove` for ordered insertion/removal
- Binary search (`lower_bound`) and prefix scanning on sorted data
- `qsort` with a comparator on a copied array
- Defensive input handling with `fgets`
- Atomic-style file replacement

## Platform notes

Standard C only; no platform-specific APIs.

## Known limitations

- Fixed field sizes: name 49, phone 20, email 60 characters. Longer input is rejected.
- Text is treated as single-byte characters; non-ASCII names work for storage but case-insensitive comparison only folds ASCII letters.
- Whole-file load/save; not designed for very large phonebooks.
- Phone numbers are stored as typed (no normalisation), so "98765 43210" and "9876543210" are different.

## Testing status

- [ ] Compile with warnings enabled and fix any diagnostics.
- [ ] Add/list/lookup/prefix/search/edit/delete behave as described.
- [ ] Rename to a name that already exists is refused; case-only rename works.
- [ ] Data persists across restarts; corrupt lines are skipped safely.
- [ ] Invalid menu input, over-long input, and end-of-input (Ctrl+Z / Ctrl+D) are handled.

**Verification note:** this source has been reviewed by reading only. It has not been compiled or executed. GitHub Actions is not used to build or run native projects.
