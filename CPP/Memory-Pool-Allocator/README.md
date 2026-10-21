# Memory Pool Allocator

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

Three allocator building blocks with tests and a benchmark: a fixed-size block `FixedPool` with an intrusive free list, an STL-compatible `PoolAllocator<T>` so `std::list`/`std::map` can allocate nodes from the pool, and a bump-pointer `ArenaAllocator` with alignment and `reset()`. It demonstrates how custom allocators remove per-allocation heap calls and why alignment matters.

## Features

- `FixedPool<BlockSize, Alignment>`: O(1) allocate/free, chunked growth, aligned chunk allocation via `operator new(std::align_val_t)`, `owns()`, double-free and foreign-pointer detection.
- `LockedPool<Pool>`: mutex wrapper for multi-threaded use.
- `PoolAllocator<T>`: C++17 allocator with `rebind`, shared pool across rebound copies, array fall-back to `new`.
- `ArenaAllocator`: aligned bump allocation, `create<T>()` with placement new, `reset()`, peak statistics, `bad_alloc` on overflow.
- Self-tests (~25 checks) and a benchmark comparing `new/delete`, pool and arena.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- Linux: add `-pthread` (std::mutex). Build with `-O2` for meaningful benchmark numbers.

## Compile

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o pool -pthread     # Linux / macOS / WSL
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o pool.exe                      # Windows MinGW
cl /std:c++17 /O2 /EHsc /W4 main.cpp /Fe:pool.exe                           # Windows MSVC
```

## Run

```bash
./pool             # tests + benchmark with 100000 objects
./pool 1000000     # bigger benchmark
```

## Usage example

```text
Running allocator self-tests...
25 passed, 0 failed.

Benchmark: 100000 x 56-byte objects, 10 rounds of alloc+free
  new/delete : 41.2 ms
  FixedPool  : 6.8 ms  (5468 KiB reserved in 98 chunks)
  Arena      : 1.1 ms  (peak 5468 KiB)
Note: results vary by platform, optimiser and system allocator; build with -O2 for meaningful numbers.
```

## File storage

None.

## Platform compatibility

Standard C++17 only; works on Windows, Linux and macOS.

## Known limitations

- `FixedPool` never returns chunks to the OS until destruction.
- The double-free check walks the free list (O(n)); it is a debugging aid, not a production feature.
- `ArenaAllocator::create<T>` does not run destructors on `reset()` — intended for trivially destructible or manually destroyed objects.
- Benchmark numbers in the example are illustrative, not measured.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Self-tests report 0 failures on Windows and Linux
- [ ] Build with `-fsanitize=address,undefined` on Linux/WSL
- [ ] Benchmark with several sizes
- [ ] Confirm `std::map` with `PoolAllocator` compiles on MSVC (allocator conformance)

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
