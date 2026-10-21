# Password Generator

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

Generates random passwords from selectable character sets (lower/upper/digits/symbols, optional removal of ambiguous characters) and memorable word-based passphrases. It guarantees at least one character from each enabled set and reports an entropy estimate.

## Features

- Length 4–128, 1–50 passwords per run.
- Per-set toggles and an *exclude ambiguous* option (`0 O o 1 l I |`).
- Guaranteed inclusion of each enabled set, then shuffled.
- Passphrase mode: 3–10 words, custom separator, optional capitalisation and number.
- Entropy estimate in bits with a weak/fair/strong/very strong label.
- Uses `std::random_device` + `std::mt19937` + `std::uniform_int_distribution` (no modulo bias).

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
=== Password Generator ===

1. Random password
2. Memorable passphrase
0. Quit
Choose [1]: 1
Length (4-128) [16]: 20
Include lowercase letters? [Y/n]:
Include uppercase letters? [Y/n]:
Include digits? [Y/n]:
Include symbols? [Y/n]: n
Exclude ambiguous characters (0 O 1 l I |)? [y/N]: y
How many passwords? [1]: 2

Entropy: ~115 bits (very strong)
  k7RwXq2PfbZ9mTcEhUva
  3GdNsYj8WeqHtMz5xKrA
```

## File storage

None.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- `std::mt19937` is seeded from `std::random_device`; this is fine for learning but for production secrets a platform CSPRNG is preferable.
- The passphrase word list is small (40 words), which limits passphrase entropy — the estimate shown is honest about this.
- Passwords are printed to the terminal; clear your screen/history afterwards.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] All set combinations including a single set
- [ ] Length smaller than number of enabled sets is rejected
- [ ] Ambiguous exclusion actually removes characters
- [ ] Passphrase separator and number options

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
