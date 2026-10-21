# Basic HTTP Server

> **Status: In Progress** — C source and documentation are available. The code has **not** yet been compiled, run, or tested.

## Overview

A small, single-threaded HTTP/1.1 **static file server** for learning how sockets work: create a socket, bind, listen, accept, read a request, and write a response. It serves files from a folder and returns proper HTTP error codes.

## Features

- `GET` and `HEAD` requests.
- Serves files from a document root (default `public/`); `/` maps to `index.html`.
- Status codes: 200, 400, 403, 404, 405, 500.
- `Content-Type` for common extensions (HTML, CSS, JS, JSON, text, PNG, JPEG, GIF, SVG, ICO).
- Path safety: only `A–Z a–z 0–9 / . _ -` are allowed in paths and any `..` is rejected (no directory traversal).
- Binds to **127.0.0.1 only**, so other machines cannot reach it.
- Receive timeout (5 s) so a silent client cannot block the server forever.
- One log line per request on the console.

## Requirements and platforms

| Platform | Support |
| --- | --- |
| Windows (MinGW-w64 / MSYS2 GCC) | Supported through **Winsock**; link with `-lws2_32` |
| Linux, macOS, WSL | Supported through BSD sockets |
| Windows with MSVC | Not covered by these instructions and not attempted |

The same `main.c` uses `#ifdef _WIN32` to select Winsock or POSIX sockets.

## Build and run

Windows (MinGW-w64):

```bash
cd C/Basic-HTTP-Server
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o http-server.exe -lws2_32
http-server.exe
```

Linux / macOS / WSL:

```bash
cd C/Basic-HTTP-Server
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o http-server
./http-server
```

Optional arguments: `http-server [port] [document_root]` (default `8080` and `public`). Run it from this folder so `public/` is found, then open <http://127.0.0.1:8080/>. Stop with `Ctrl+C`.

If a strict `-std=c11` build reports missing socket declarations on your toolchain, try `-std=gnu11` and please note it as an issue.

## Usage examples

```bash
curl -i http://127.0.0.1:8080/            # expect 200 and the sample page
curl -i http://127.0.0.1:8080/hello.txt   # expect 200, text/plain
curl -I http://127.0.0.1:8080/hello.txt   # HEAD: headers only
curl -i http://127.0.0.1:8080/nope.html   # expect 404
curl -i -X POST http://127.0.0.1:8080/    # expect 405 with an Allow header
curl -i --path-as-is http://127.0.0.1:8080/../main.c   # expect 400
```

These are the **intended** behaviours from reading the code; none have been run yet.

## Included sample files

`public/index.html` and `public/hello.txt` let you try the server immediately.

## File storage

The server only reads files from the document root. It never writes files.

## Concepts practised

- Socket lifecycle (`socket`, `bind`, `listen`, `accept`, `recv`, `send`, close)
- HTTP request line and response header format
- Partial `send()` handling and chunked file reads
- Input sanitising to prevent path traversal
- Writing portable code with conditional compilation

## Known limitations

- Handles **one client at a time**; no threads and no keep-alive (`Connection: close`).
- Does not decode percent-encoding (e.g. `%20`), so such paths are rejected as 400.
- Only the request line is parsed; request headers and bodies are ignored.
- Request line (first 8 KB of headers) must fit in the buffer; very long URLs are rejected.
- No HTTPS, authentication, range requests, caching headers, or directory listings.
- Files larger than 512 MB return 500; file sizes use `unsigned long`, which is 32-bit on Windows.
- Intended for local learning only. Do not expose it to the internet.

## Testing status

- [ ] Compiles on Windows (MinGW-w64) with `-lws2_32`.
- [ ] Compiles on Linux/WSL.
- [ ] Sample page, text file, 404, 405, and `HEAD` behave as described.
- [ ] Path traversal attempts are rejected.
- [ ] Large file and early-disconnect behaviour checked.

**Verification note:** this source has been reviewed by reading only. It has not been compiled or executed. GitHub Actions is not used to build or run native projects.
