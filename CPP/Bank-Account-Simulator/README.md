# Bank Account Simulator

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

Simulates a single bank account with deposits, withdrawals, an overdraft limit, pro-rata interest and a transaction statement. All money is stored as integer cents so balances are exact, and every rule violation is reported through exceptions with clear messages.

## Features

- Deposit, withdraw (with overdraft limit), balance enquiry, interest application, overdraft adjustment.
- Exact money arithmetic using `long long` cents and a strict amount parser (`25`, `25.5`, `25.50`).
- Full transaction history with running balance and notes.
- Encapsulated `BankAccount` class; invariants enforced in one place.
- Separate `invalid_argument` (bad input) and `runtime_error` (business rule) handling.

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
=== Bank Account Simulator ===
Account holder name: Grace
Opening deposit (0 for none): 100
Account opened for Grace with $100.00.

1. Deposit ... Choose: 2
Amount: 180
Done. Balance: -$80.00
Choose: 2
Amount: 50
Declined: Insufficient funds: available $20.00 (including overdraft)
Choose: 6

Statement for Grace
Type              Amount         Balance  Note
------------------------------------------------------------
Open             $100.00         $100.00  Opening deposit
Withdraw        -$180.00         -$80.00  Overdraft used
```

## File storage

None — state lives in memory for the session only (by design for this beginner project; see Bank Management System for persistence).

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Single account, no persistence, no authentication.
- Interest is simple pro-rata, not compounded.
- Amounts above $1,000,000,000 are rejected by the parser.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Deposit/withdraw happy paths
- [ ] Withdrawal beyond overdraft is declined
- [ ] Malformed amounts (`-5`, `1.234`, `abc`)
- [ ] Interest with zero or negative balance

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
