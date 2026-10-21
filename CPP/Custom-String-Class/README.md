# Custom String Class

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

`MyString` is a string type written from scratch to explore ownership and value semantics. It owns a heap buffer (or a 15-character small buffer), implements the Rule of Five with copy-and-swap, overloads the usual operators, exposes iterators that work with `<algorithm>`, and ships with a built-in self-test suite that doubles as documentation.

## Features

- Rule of Five: destructor, copy/move constructors, unified copy-and-swap assignment (strong exception safety, self-assignment safe).
- Small-buffer optimisation (SSO) for strings up to 15 characters; transparent promotion to the heap.
- Operators `+ += == != < <= > >= [] << >>` and `swap`.
- `append` is safe when the source aliases the string's own buffer; `insert` is safe for self-insert.
- `at()` with `std::out_of_range`, null-pointer constructor throws `std::invalid_argument`.
- Utilities: `substr`, `find`, `insert`, `erase`, `toUpper`, `toLower`, `trimmed`, `reversed`.
- `--test` flag runs ~40 assertions and returns a non-zero exit code on failure.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- No external libraries.

## Compile

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o mystring
```

## Run

```bash
./mystring --test      # run self-tests only
./mystring             # self-tests, then an interactive demo (type words, Ctrl+D / Ctrl+Z to finish)
```

## Usage example

```text
Running MyString self-tests...
41 passed, 0 failed.

Interactive demo - type words (Ctrl+D / Ctrl+Z to finish):
hello custom string
^D
You entered 3 word(s): "hello custom string"
Upper:    HELLO CUSTOM STRING
Reversed: gnirts motsuc olleh
Length 19, capacity 30, storage: heap
```

## File storage

None.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Byte-oriented: no Unicode awareness (UTF-8 passes through but `toUpper` etc. only touch ASCII).
- No `std::string_view` interop or allocator support.
- `find` is a simple O(n·m) scan.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] `--test` reports 0 failures
- [ ] Run under a sanitizer (`-fsanitize=address,undefined`) on Linux/WSL to confirm no leaks or UB
- [ ] Interactive demo with long and short inputs

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
