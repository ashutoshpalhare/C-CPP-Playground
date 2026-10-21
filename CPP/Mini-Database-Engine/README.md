# Mini Database Engine

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A small relational database engine with a SQL-like language: typed tables, primary keys, secondary indexes, filtering, ordering, limits, updates and deletes, plus a hand-written tokenizer and recursive-descent parser. Data persists to a text file (`mini.db` by default).

## Features

- `CREATE TABLE` with `INT`/`TEXT` columns and one `PRIMARY KEY`; `DROP TABLE`, `SHOW TABLES`, `DESCRIBE`.
- `INSERT` (multi-row, optional column list), `SELECT` with `WHERE`, `ORDER BY ... ASC|DESC`, `LIMIT`, `UPDATE ... SET ... WHERE`, `DELETE FROM ... WHERE`.
- `CREATE INDEX ON table (column)`; equality lookups on indexed columns use a `std::multimap` index (reported as *index used*).
- Type checking, duplicate-key detection, helpful error messages with the offending token.
- `SAVE`/`LOAD` to a simple line-based file; prompt to save on exit when there are unsaved changes.
- `std::variant` cell values, `std::unique_ptr` table ownership.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- No external libraries.

## Compile

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o minidb
```

## Run

```bash
./minidb              # uses mini.db
./minidb shop.db      # custom file
```

## Usage example

```text
db> CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)
Table users created with 3 column(s).
db> INSERT INTO users VALUES (1, 'Ada', 36), (2, 'Linus', 28), (3, 'Grace', 85)
3 row(s) inserted.
db> SELECT name, age FROM users WHERE age > 30 ORDER BY age DESC
+-------+-----+
| name  | age |
+-------+-----+
| Grace |  85 |
| Ada   |  36 |
+-------+-----+
2 row(s)
db> CREATE INDEX ON users (name); SELECT * FROM users WHERE name = 'Ada'
Index created on users(name).
+----+------+-----+
| id | name | age |
+----+------+-----+
|  1 | Ada  |  36 |
+----+------+-----+
1 row(s) (index used)
db> UPDATE users SET age = 37 WHERE id = 1
1 row(s) updated.
db> SAVE
Saved to mini.db.
```

## File storage

Text file (default `mini.db`): per table a `TABLE name ncols` header, one line per column (`name TYPE pk`), an `INDEXES` line, a `ROWS n` line and tab-separated rows (tabs/newlines/backslashes escaped). Rewritten entirely on `SAVE`.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- No joins, aggregates (`COUNT`, `SUM`), `NULL`, `AND`/`OR` in `WHERE` (one condition only), or transactions.
- Whole-table rewrite on save; everything lives in memory.
- Indexes are rebuilt after `UPDATE`/`DELETE` (simple, O(n)).
- Identifiers are case-sensitive; keywords are not.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Each statement type incl. error paths (bad types, duplicate PK, unknown column)
- [ ] `ORDER BY` + `LIMIT` combinations
- [ ] Index used for equality, scan for ranges
- [ ] SAVE then restart + automatic LOAD round-trip
- [ ] Strings containing quotes (`'it''s'`) and tabs

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
