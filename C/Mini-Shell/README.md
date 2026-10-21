# Mini Shell

> **Status: In Progress** — C source and documentation are available. The code has **not** yet been compiled, run, or tested.

## Platform requirement: POSIX only

This project uses `fork`, `execvp`, `pipe`, `dup2`, `waitpid`, and `sigaction`. It runs on **Linux, macOS, and Windows through WSL**. It **does not build natively on Windows** (MinGW or MSVC); on Windows the source stops with a clear `#error`.

To use it on Windows, install WSL (`wsl --install` in an administrator terminal), then inside the Linux shell: `sudo apt update && sudo apt install build-essential`.

## Overview

A small command interpreter that shows how a real shell starts programs: read a line, split it into words, create processes, connect their input and output, and wait for them.

## Features

- Runs external programs found via `PATH`.
- Pipelines: `ls | sort | head` (up to 16 commands).
- Redirection: `< input`, `> output` (truncate), `>> output` (append).
- Background jobs with a trailing `&`; finished jobs are reported before the next prompt.
- Quoting: `'single quotes'`, `"double quotes"` (with `\"` and `\\`), and backslash escapes.
- Built-ins: `cd`, `pwd`, `exit [code]`, `help`, `history`, `status`.
- The shell ignores `Ctrl+C`; foreground programs get the default action; background jobs ignore it.
- Exit status tracking (`status` shows the last command's code; 127 = not found, 126 = cannot execute, 128+N = killed by signal N).

## Build and run

```bash
cd C/Mini-Shell
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o mini-shell
./mini-shell
```

## Usage examples

```text
msh:/home/user$ echo "hello   world" > out.txt
msh:/home/user$ cat out.txt | wc -c
msh:/home/user$ sleep 3 &
[background] pid 12345
msh:/home/user$ cd /tmp
msh:/tmp$ status
0
```

(Illustrative; not yet run.)

## Concepts practised

- Process creation and replacement (`fork`/`exec`)
- File descriptors, `dup2`, and pipes (and closing unused ends so pipelines finish)
- Waiting for children and reaping background jobs
- Signal dispositions across `fork` and `exec`
- A tokenizer and a small parser for pipelines and redirections

## Known limitations

- No environment-variable expansion (`$HOME`), wildcards (`*.c`), `~`, command substitution, `&&`, `||`, or `;`.
- No job control (`fg`, `bg`, `jobs`) and no process groups or terminal control; background jobs share the shell's process group.
- Built-ins cannot be piped, redirected, or backgrounded.
- `history` is in-memory only (last 100 lines) and has no `!!` expansion or arrow-key editing.
- Lines are limited to 1023 characters and 127 words.
- `&` is only accepted as the last token.

## Testing status

- [ ] Compiles on Linux or WSL with warnings enabled.
- [ ] External commands, pipelines, and redirections work; pipelines do not hang.
- [ ] Background jobs run and are reported; no zombie processes remain.
- [ ] `Ctrl+C` stops a foreground command but not the shell.
- [ ] Syntax errors, unknown commands, and unterminated quotes are reported safely.

**Verification note:** this source has been reviewed by reading only. It has not been compiled or executed. GitHub Actions is not used to build or run native projects.
