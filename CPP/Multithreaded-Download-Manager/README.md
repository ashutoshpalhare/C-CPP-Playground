# Multithreaded Download Manager

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A command-line download manager that fetches several files at once using a thread pool and, when a server advertises `Accept-Ranges: bytes`, downloads each file in parallel segments with HTTP `Range` requests written directly into a pre-sized output file. It uses raw sockets with a minimal HTTP/1.1 client, so it supports plain `http://` URLs and local files (for offline testing) but **not** `https://`.

## Features

- Thread pool with job queue, `std::condition_variable` idle-wait, and a progress reporter thread.
- Per-file segmentation (`-s`), concurrent downloads (`-j`), output directory (`-o`), URL list file (`-f`).
- HEAD probe for size and range support; falls back to single-stream download.
- Per-segment retries with progress rollback; failed downloads are removed.
- Positioned writes guarded by a mutex; throughput and timing summary.
- Local-file sources exercise the same segmented pipeline without a network.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- **Windows:** MinGW-w64 or MSVC; link `ws2_32`.
- **Linux/macOS/WSL:** `-pthread`.
- A plain-HTTP server to test against (e.g. this repository's C *Basic HTTP Server*, or `python -m http.server 8000`).

## Compile

```bash
# Linux / macOS / WSL
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o dlm -pthread

# Windows (MinGW-w64)
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o dlm.exe -lws2_32

# Windows (MSVC)
cl /std:c++17 /O2 /EHsc /W4 main.cpp ws2_32.lib /Fe:dlm.exe
```

## Run

```bash
# offline test with local files (segmented copy)
./dlm -s 4 -o out ./bigfile.bin ./another.iso

# plain HTTP (python -m http.server supports Range requests)
./dlm -j 2 -s 4 -o downloads http://127.0.0.1:8000/bigfile.bin http://127.0.0.1:8000/other.zip

# list file
./dlm -f urls.txt
```

## Usage example

```text
start  bigfile.bin  size 48.0 MiB  (ranges supported, 4 segments)
start  other.zip  size 3.2 MiB  (single stream)
progress:  bigfile.bin 37%  other.zip 100%
done   other.zip  3.2 MiB in 0.41 s  (7.8 MiB/s)
progress:  bigfile.bin 81%  other.zip 100%
done   bigfile.bin  48.0 MiB in 2.63 s  (18.2 MiB/s)
Finished: 2 ok, 0 failed.
```

## File storage

Downloaded files are written to the output directory (default `.`) using the last path component of the URL as the filename (unsafe characters replaced by `_`). Existing files with the same name are overwritten.

## Platform compatibility

| Platform | Supported | Notes |
| --- | --- | --- |
| Windows 10/11 | Yes | Winsock2 (`-lws2_32`). |
| Linux / WSL / macOS | Yes | BSD sockets, `-pthread`. |

Network: outbound TCP to the target host. Works through plain HTTP proxies only if the URL points at the proxy.

## Known limitations

- **No HTTPS** (no TLS without an external library) — most public URLs are https and will be skipped with a message.
- Redirects (3xx) are not followed; use the final URL.
- Chunked transfer encoding is not supported for single-stream downloads (servers normally send `Content-Length` for static files).
- Files larger than 2 GiB may not work on platforms where `long` is 32-bit (`fseek` limit).
- No resume of partially downloaded files between runs.

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Local-file segmented copy produces byte-identical output (`fc /b` / `cmp`)
- [ ] Download from `python -m http.server` with ranges
- [ ] Server without range support falls back to single stream
- [ ] Unreachable host and 404 are reported as failures
- [ ] Interrupting the server mid-download triggers retries then failure

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
