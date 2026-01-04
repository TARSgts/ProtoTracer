#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path


def encode_frame(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        run_len = 1
        while i + run_len < n and data[i + run_len] == data[i] and run_len < 128:
            run_len += 1

        if run_len >= 3:
            out.append(0x80 | (run_len - 1))
            out.append(data[i])
            i += run_len
            continue

        start = i
        i += run_len
        while i < n:
            run_len = 1
            while i + run_len < n and data[i + run_len] == data[i] and run_len < 128:
                run_len += 1
            if run_len >= 3:
                break
            if (i - start) + run_len > 128:
                break
            i += run_len

        literal_len = i - start
        out.append(literal_len - 1)
        out.extend(data[start:i])

    return bytes(out)


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert BAPLE.BIN into RLE stream.")
    parser.add_argument("input", type=Path, help="Input .BIN file (1-bit packed frames).")
    parser.add_argument("output", type=Path, help="Output .RLE file.")
    parser.add_argument("--frame-bytes", type=int, default=256, help="Bytes per frame.")
    args = parser.parse_args()

    raw = args.input.read_bytes()
    frame_bytes = args.frame_bytes
    if len(raw) % frame_bytes != 0:
        print(f"Input size {len(raw)} not divisible by {frame_bytes}.", file=sys.stderr)
        return 1

    frame_count = len(raw) // frame_bytes
    out = bytearray()
    for frame in range(frame_count):
        start = frame * frame_bytes
        end = start + frame_bytes
        out.extend(encode_frame(raw[start:end]))

    args.output.write_bytes(out)

    in_kb = len(raw) / 1024.0
    out_kb = len(out) / 1024.0
    ratio = (out_kb / in_kb) * 100.0 if in_kb else 0.0
    print(f"Frames: {frame_count}")
    print(f"Input:  {in_kb:.1f} KB")
    print(f"Output: {out_kb:.1f} KB ({ratio:.1f}% of original)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
