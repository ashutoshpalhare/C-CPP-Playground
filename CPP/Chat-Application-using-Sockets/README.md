# Chat Application using Sockets

> **Status: In Progress** — complete C++ source and documentation are present. The program has **not yet been compiled or run**; local verification on Windows is pending.

## Overview

A TCP chat room in a single executable. Run it once as the **server**, then any number of times as **clients** (on the same machine or across a LAN). The server broadcasts every message to all connected clients and supports nicknames; the client uses one thread for incoming messages and the main thread for typing.

## Features

- Server: thread-per-client, mutex-protected client table, broadcast, join/leave notices.
- Commands: `/nick <name>`, `/list`, `/quit`.
- Client: hostname resolution via `getaddrinfo`, reader thread, clean shutdown.
- Line-based protocol with length limits and control-character sanitising.
- Winsock2 on Windows, BSD sockets on POSIX, behind a tiny abstraction layer.

## Language and prerequisites

- C++17 compiler: GCC 8+/MinGW-w64, Clang 7+, or MSVC 2019+.
- **Windows:** MinGW-w64 or MSVC, link Winsock (`-lws2_32` / `ws2_32.lib`). Allow the program through Windows Firewall when prompted if you want LAN clients.
- **Linux/macOS/WSL:** `-pthread`.

## Compile

```bash
# Linux / macOS / WSL
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o chat -pthread

# Windows (MinGW-w64)
g++ -std=c++17 -Wall -Wextra main.cpp -o chat.exe -lws2_32

# Windows (MSVC Developer Prompt)
cl /std:c++17 /EHsc /W4 main.cpp ws2_32.lib /Fe:chat.exe
```

## Run

```bash
# Terminal 1 - server (default port 5555)
./chat server
./chat server 6000

# Terminal 2, 3, ... - clients
./chat client 127.0.0.1
./chat client 192.168.1.20 6000 alice
```

## Usage example

```text
$ ./chat server
Chat server listening on port 5555. Press Ctrl+C to stop.
Connection from 127.0.0.1:50212
* guest1 joined the chat
* guest1 is now known as alice
[alice] hello everyone

$ ./chat client 127.0.0.1 5555 alice
Connected to 127.0.0.1:5555. Type messages and press Enter. /quit to leave.
* Welcome! You are guest1. Commands: /nick <name>, /list, /quit
* guest1 is now known as alice
hello everyone
[alice] hello everyone
/list
* Online (1): alice
```

## File storage

None; nothing is logged to disk.

## Platform compatibility

| Platform | Supported | Notes |
| --- | --- | --- |
| Windows 10/11 | Yes | Winsock2; link `ws2_32`. |
| Linux / WSL | Yes | WSL2 clients can reach a Windows server via the host IP; the reverse needs WSL port forwarding. |
| macOS | Yes | BSD sockets. |

Networking: IPv4, TCP. For clients on other machines, open the chosen port in the server's firewall and use the server's LAN IP.

## Known limitations

- Plain text — no TLS/encryption or authentication; use on trusted networks only.
- Thread-per-client; fine for a classroom, not for thousands of users.
- Server stops with Ctrl+C (no graceful `/shutdown` command).
- IPv4 only (change `AF_INET` to `AF_UNSPEC` on the server to experiment with IPv6).

## Testing status

- [ ] Compile with warnings enabled (pending — not yet compiled)
- [ ] Server + two local clients exchanging messages
- [ ] `/nick` collisions and invalid names
- [ ] Client disconnect (Ctrl+C) is announced and does not crash the server
- [ ] LAN test between two machines on Windows

No compilation or execution has been performed yet. GitHub Actions is not used for native builds.
