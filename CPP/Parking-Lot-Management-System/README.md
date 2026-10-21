# Parking Lot Management System

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

Manages a 15-slot parking lot with small/medium/large bays, vehicle entry and exit tickets, time-based fees with a free grace period and a daily cap, a searchable plate index and an event log. Parked vehicles survive restarts.

## Features

- Slot sizes with fit rules (motorcycle → any; car → medium/large; van → large) and smallest-fit allocation.
- Entry/exit by licence plate (normalised to upper-case alphanumerics).
- Fees: free first 15 min, per started hour by vehicle type, capped per 24 h.
- Lot status with elapsed time per vehicle; live fee preview for a parked plate.
- `parking_state.txt` for current occupancy, `parking_log.txt` as an append-only audit trail.
- `std::optional` for free slots/results, `std::map` plate index.

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
=== Parking Lot Management ===
Choose: 1
Licence plate: ka-01 ab 1234
Vehicle type (m=motorcycle, c=car, v=van): c
Ticket issued: KA01AB1234 -> slot 5 at 2025-03-04 09:12.
Choose: 2
Licence plate: KA01AB1234
Exit: KA01AB1234 (Car) from slot 5
Parked 2h 05m, fee due: 7.50
```

## File storage

`parking_state.txt` (`slot|plate|type|entryEpochSeconds`) rewritten after every change; `parking_log.txt` appended with `time|EVENT|plate|type|slot|seconds|feeCents`.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- Lot layout (4 small, 8 medium, 3 large) and rates are constants in the source.
- Fees use the system clock; changing the clock changes fees.
- Session revenue counter resets on restart (the log keeps the full record).

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Fit rules and smallest-slot preference
- [ ] Grace period, hourly rounding and daily cap
- [ ] Duplicate plate entry rejected
- [ ] State reload after restart

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
