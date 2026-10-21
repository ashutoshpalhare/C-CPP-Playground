# Bank Management System

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A multi-account banking back-office: open savings or current accounts with a PIN, deposit, withdraw, transfer between accounts, apply month-end interest/fees, and inspect an append-only ledger. Demonstrates polymorphism with an abstract `Account` base class.

## Features

- `SavingsAccount` (interest, no overdraft) and `CurrentAccount` ($500 overdraft, monthly fee) via virtual functions.
- PIN-protected operations; closing requires a zero balance.
- Atomic-feeling transfers: the withdrawal is validated before the deposit is applied.
- Append-only `ledger.txt` with timestamps; per-account history view.
- Money stored as integer cents with a strict parser.
- Saves after every successful operation.

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
=== Bank Management System ===
1. Open account ... Choose: 1
Type (s=savings, c=current): s
Holder name: Alan
4-digit PIN: 4321
Initial deposit (0 for none): 250
Savings account 1001 opened with balance 250.00.
Choose: 4
Account number: 1001
PIN: 4321
Amount: 300
Destination account: 1002
Declined: Insufficient funds (minimum balance 0.00)
```

## File storage

`accounts.txt` (`number|S/C|holder|pin|balanceCents`) and `ledger.txt` (`number|timestamp|kind|deltaCents|balanceAfterCents|note`), both in the working directory.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- PINs are stored in plain text — this is a learning project, not a secure system.
- No concurrency control; one process at a time.
- Interest/fee rates are constants in the source.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Open/deposit/withdraw/transfer/close flows
- [ ] Wrong PIN and unknown account handling
- [ ] Overdraft limit on current accounts
- [ ] Month-end on positive and negative balances

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
