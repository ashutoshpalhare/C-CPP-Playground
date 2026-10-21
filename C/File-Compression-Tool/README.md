# File Compression Tool

> **Status: In Progress** — C source and documentation are available. The code has **not** yet been compiled, run, or tested.

## Overview

A command-line **Huffman coding** compressor and decompressor. Frequent byte values get short bit patterns and rare ones get longer patterns, so files with uneven byte frequencies (such as plain text) can shrink.

## Features

- `compress` and `decompress` commands (short forms `c` and `d`) for any file type.
- Deterministic Huffman tree so the decoder rebuilds the same tree from a stored frequency table.
- Handles edge cases: empty files and files containing only one distinct byte value.
- Validates the magic header; detects truncated compressed data.
- Removes incomplete output files after a failure.
- Refuses to use the same path for input and output.
- Prints original size, compressed size, and ratio.

## Requirements

- C11 compiler. Standard C only, so it works on Windows, Linux, and macOS.

## Build and run

```bash
cd C/File-Compression-Tool
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o huffc
./huffc compress   notes.txt notes.huf      # Windows: huffc.exe compress ...
./huffc decompress notes.huf notes_copy.txt
```

## File format (`HUF1`, little-endian)

| Bytes | Content |
| --- | --- |
| 0–3 | Magic `HUF1` |
| 4–1027 | 256 × `uint32` — occurrences of each byte value |
| 1028… | Huffman bit stream, most significant bit first, zero-padded to a full byte |

The original length is the sum of the 256 counts, so it is not stored separately.

## Usage examples

```text
> huffc compress book.txt book.huf
Original size  : 120000 bytes
Compressed size: 70000 bytes (includes the 1028-byte header)
Ratio          : 58.3% of original
```

(The numbers above are illustrative only; the tool has not been run yet.) To check a round trip, decompress and compare with the original (`fc /b` on Windows, `cmp` on Linux/macOS).

## Concepts practised

- Building a Huffman tree with a priority-style selection and deterministic tie-breaking
- Generating prefix codes by tree traversal
- Bit-level output (shifting, masking, padding) and bit-by-bit decoding
- Binary file I/O and explicit little-endian integer encoding
- Cleaning up partial output on errors

## Known limitations

- The 1 KiB header means **small files usually get larger**; already-compressed or random data will not shrink.
- Input must be smaller than 4 GiB (counts are 32-bit).
- The format is custom to this project and is not compatible with zip, gzip, or other tools.
- No checksum: corruption inside the bit stream may decode to wrong data instead of an error.
- Reads the input twice, so it cannot compress from a pipe or standard input.
- Tree construction is O(n²) over at most 256 symbols; this is negligible in practice.

## Testing status

- [ ] Compiles with warnings enabled on Windows.
- [ ] Round trip (compress then decompress) matches the original for text, binary, empty, one-byte, single-repeated-byte, and large files.
- [ ] Bad header, truncated file, missing input, and same-path arguments are handled.
- [ ] Compression ratio on typical text is plausible.

**Verification note:** this source has been reviewed by reading only. It has not been compiled or executed. GitHub Actions is not used to build or run native projects.
