#!/usr/bin/env python3
"""
Standalone mirror-focused entry point for ProtoTracer USB streaming.

Behavior:
- If no CLI args are provided, mirror the primary monitor at 30 FPS.
- If args are provided, forward them directly to usb_video_streamer.py.
"""

from __future__ import annotations

import sys

import usb_video_streamer


DEFAULT_ARGS = ["--source", "screen", "--monitor", "primary", "--fps", "30"]


def main() -> int:
    forwarded = sys.argv[1:] if len(sys.argv) > 1 else DEFAULT_ARGS
    sys.argv = [sys.argv[0], *forwarded]
    return usb_video_streamer.main()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nStopped.")
        raise SystemExit(0)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)
