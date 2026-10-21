# Memory Allocator Simulator

> **Status: In Progress** — C source and documentation are available. The code has **not** yet been compiled, run, or tested.

## Overview

An interactive simulator of a heap allocator. The "heap" is only a list of block descriptors (start, size, owner), so it **does not replace `malloc`** and cannot corrupt real memory. Use it to see how placement strategies and freeing create fragmentation.

## Features

- First-fit, best-fit, and worst-fit strategies (switchable at any time).
- Block splitting when a free block is larger than needed.
- Coalescing of adjacent free blocks on `free`.
- 8-byte alignment (requests are rounded up).
- Text map of the heap plus a block table.
- Statistics: used/free bytes, largest free block, internal waste, **external fragmentation** (`1 − largest free ÷ total free`), success/failure counts.
- Built-in `demo` that replays one scenario under all three strategies.
- Validated commands; heap size between 64 and 1,048,576 bytes.

## Requirements

- C11 compiler. Standard C only; works on Windows, Linux, and macOS.

## Build and run

```bash
cd C/Memory-Allocator-Simulator
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o allocator-sim
./allocator-sim          # Windows: allocator-sim.exe
```

## Commands

| Command | Meaning |
| --- | --- |
| `alloc N` | Request N bytes; prints a block id |
| `free ID` | Release a block |
| `strategy first\|best\|worst` | Choose placement strategy |
| `show` | Block table and heap map |
| `stats` | Usage and fragmentation figures |
| `reset [SIZE]` | New heap (default 1024), keeps the chosen strategy |
| `demo` | Compare strategies on a scripted scenario |
| `help`, `quit` | Help / leave |

## Example session

The output below was derived by hand from the program's print formats and the allocation rules; it has not been produced by running the program.

```text
alloc> reset 256
Heap reset to 256 bytes.
alloc> alloc 30
Allocated block 1 (30 bytes requested).
alloc> alloc 100
Allocated block 2 (100 bytes requested).
alloc> free 1
Freed block 1.
alloc> show
Heap: 256 bytes | strategy: first-fit | alignment: 8
[........22222222222222222222222222..............................]
('.' = free, digit = last digit of the owning block id)
ID       Start      End     Size  State     Requested
-            0       31       32  free      
2           32      135      104  allocated 100
-          136      255      120  free      
```

Each character of the map represents 4 bytes of this 256-byte heap.

## Concepts practised

- Singly linked lists, splitting and merging nodes
- Placement policies and their trade-offs
- Alignment arithmetic and rounding
- Measuring fragmentation
- Parsing and validating text commands

## Known limitations

- Simulation only: no real memory is allocated or returned to you.
- Block metadata is not stored inside the simulated heap (real allocators embed headers), so no per-block overhead is modelled.
- No next-fit, buddy, or slab strategies; no compaction.
- Allocation ids are never reused within a heap.
- Single-threaded and interactive; `demo` uses a fixed scenario.

## Testing status

- [ ] Compiles with warnings enabled.
- [ ] Split, coalesce (both neighbours), and exhaustion cases behave correctly.
- [ ] All three strategies choose the expected block in hand-made scenarios.
- [ ] Invalid commands, huge numbers, and non-numeric IDs are rejected.
- [ ] `demo` output is consistent with manual replay.

**Verification note:** this source has been reviewed by reading only. It has not been compiled or executed. GitHub Actions is not used to build or run native projects.
