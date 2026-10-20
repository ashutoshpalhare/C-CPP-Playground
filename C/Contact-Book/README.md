# Contact Book

> **Status: In Progress** — initial C source and usage documentation are available. Compilation and functional testing are deferred until the planned project implementations are added.

## Overview

A menu-driven terminal contact manager. Add, list, search, and delete contacts. Data is stored locally in `contacts.csv` so entries remain available when the program is restarted.

## Features

- Add contacts with an automatically assigned positive ID, name, phone, and email.
- List saved contacts.
- Search by a substring of name or phone number.
- Delete a contact by ID.
- Validate required fields, field lengths, and numeric menu choices.
- Handle missing files and common file-operation errors.

## Requirements

- C11-compatible compiler such as GCC or Clang.
- Terminal with write permission in the working directory.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o contact-book
./contact-book
```

On Windows, run `contact-book.exe`.

## Data storage

The app creates `contacts.csv` in the current working directory. Each line contains:

```text
ID,Name,Phone,Email
```

This is a deliberately simple comma-separated format, not a full CSV implementation: fields cannot contain commas. Keep a backup before manually editing the data file. Contact IDs are assigned using the highest valid ID currently present plus one; concurrent program instances are not supported.

## Example session

```text
1. Add contact
2. List contacts
3. Search by name or phone
4. Delete contact
5. Exit
Choose an option: 1
New contact ID: 1
Name: Asha Patil
Phone: 9876543210
Email: asha@example.com
Contact added successfully.
```

## Concepts practised

- Structures and string handling
- File input/output
- Searching and record parsing
- Input validation and menu-driven control flow
- Temporary-file replacement for deletion

## Limitations

- Search is case-sensitive.
- Names, phone numbers, and email addresses cannot contain commas.
- This is a local single-user app without a database or concurrent access protection.
- Deletion uses a temporary file; keep backups of important records.

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Add contacts and verify persistence after restarting.
- [ ] List contacts and search by name/phone fragments.
- [ ] Delete a contact and confirm other records remain.
- [ ] Test empty fields, long fields, invalid menu choices, and missing files.

**Verification note:** The source has not yet been compiled or run. Local verification is deferred until the initial project implementations are added across the collection. GitHub Actions is not used to compile or execute native C programs.
