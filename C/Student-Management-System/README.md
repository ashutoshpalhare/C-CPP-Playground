# Student Management System

> **Status: In Progress** — initial C source and documentation are available. Local compilation and testing are deferred until the planned project implementations have been added.

## Overview

A menu-driven terminal app for managing student records. Add, list, update, and delete student entries. Records persist between runs in a local `students.csv` file in the current working directory.

## Features

- Add students with a unique positive integer ID, name, and marks from 0 to 100.
- List saved student records.
- Update a student's name and/or marks by ID.
- Delete a student record by ID.
- Validate IDs, marks, menu selections, and names.
- Report missing files and common file-operation errors.

## Requirements

- C compiler supporting C11 (GCC or Clang).
- Terminal with read/write permission in the working directory.

## Build and run

From this directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o student-management
./student-management
```

On Windows, run `student-management.exe`.

## Data format and storage

The application creates `students.csv` in the directory from which it is launched. Each record is stored as:

```text
ID,Name,Marks
```

The first line is a student record, not a header row. Names must be non-empty and must not contain commas because this intentionally small CSV format does not implement quoted CSV fields. Keep a backup of `students.csv` before manually editing it. Updates and deletions use a temporary `students.tmp` file.

## Example session

```text
1. Add student
2. List students
3. Update student
4. Delete student
5. Exit
Choose an option: 1
Student ID (positive integer): 101
Student name (commas are not allowed): Asha Patil
Marks (0-100): 88.5
Student added successfully.
```

## Concepts practised

- Structures and functions
- Persistent file I/O
- Record lookup and unique IDs
- Numeric and string validation
- Menu-driven program design
- Temporary-file updates

## Limitations

- Single-user, local-file application; no database or concurrent access support.
- Names cannot contain commas.
- The simple temporary-file replacement strategy is not a transactional storage system; back up important data before editing records.
- Existing malformed records may be skipped when listing or rewritten unchanged during updates/deletions.

## Local verification checklist

- [ ] Compile with C11 and warnings enabled.
- [ ] Add records and verify they persist after restarting.
- [ ] List records, update name/marks, and delete by ID.
- [ ] Test duplicate IDs, missing IDs, invalid marks, and invalid menu choices.
- [ ] Test empty/overlong names and file permission errors.

**Verification note:** This implementation has not yet been compiled or run. Testing is deferred until the initial source implementations are added across the collection. GitHub Actions is not used to compile or execute native C programs.
