#!/usr/bin/env python3
"""
Streams screen/camera/video frames to ProtoTracer over USB serial.

Protocol:
  Packet = 'P' 'T' + 6-byte header + payload
  Header = version(1), flags(1), sequence(LE16), payload_length(LE16)
  Payload = 64x32 RGB565 little-endian (4096 bytes)

Flags:
  bit 0: force mirror to lower panel
  bit 1: force single panel (disable lower panel mirror)
  bit 7: control packet (payload = command byte)
"""

from __future__ import annotations

import argparse
import struct
import sys
import time
from typing import Optional, Tuple, Union

try:
    import cv2
    import mss
    import numpy as np
    import serial
    import serial.tools.list_ports
except ImportError as exc:
    missing = getattr(exc, "name", str(exc))
    sys.stderr.write(
        f"Missing dependency: {missing}\n"
        "Install dependencies with:\n"
        "  pip install -r tools/usb_video_streamer_requirements.txt\n"
    )
    raise SystemExit(1) from exc

PROTO_MAGIC = b"PT"
PROTO_VERSION = 1
FLAG_MIRROR_LOWER = 0x01
FLAG_SINGLE_PANEL = 0x02
FLAG_CONTROL_PACKET = 0x80

CMD_START_STREAM_MODE = 0x01
CMD_STOP_STREAM_MODE = 0x02

TARGET_WIDTH = 64
TARGET_HEIGHT = 32
PAYLOAD_SIZE = TARGET_WIDTH * TARGET_HEIGHT * 2


class FrameSource:
    def read(self) -> Optional[np.ndarray]:
        raise NotImplementedError

    def close(self) -> None:
        return None


class ScreenSource(FrameSource):
    def __init__(self, monitor_index: int, region: Optional[Tuple[int, int, int, int]]) -> None:
        self._capture = mss.mss()

        if region is not None:
            left, top, width, height = region
            self._rect = {"left": left, "top": top, "width": width, "height": height}
            return

        monitors = self._capture.monitors
        if len(monitors) <= 1:
            raise RuntimeError("No monitors found by mss.")
        if monitor_index < 1 or monitor_index >= len(monitors):
            raise RuntimeError(
                f"Monitor index {monitor_index} is out of range. Use --list-monitors to view valid indices."
            )

        monitor = monitors[monitor_index]
        self._rect = {
            "left": int(monitor["left"]),
            "top": int(monitor["top"]),
            "width": int(monitor["width"]),
            "height": int(monitor["height"]),
        }

    def read(self) -> Optional[np.ndarray]:
        frame = np.asarray(self._capture.grab(self._rect), dtype=np.uint8)
        return frame[:, :, :3]

    def close(self) -> None:
        self._capture.close()


class VideoCaptureSource(FrameSource):
    def __init__(self, source: Union[int, str], loop: bool = False) -> None:
        self._capture = cv2.VideoCapture(source)
        if not self._capture.isOpened():
            raise RuntimeError(f"Could not open capture source: {source}")
        self._loop = loop

    def read(self) -> Optional[np.ndarray]:
        ok, frame = self._capture.read()
        if ok:
            return frame

        if self._loop:
            self._capture.set(cv2.CAP_PROP_POS_FRAMES, 0)
            ok, frame = self._capture.read()
            if ok:
                return frame

        return None

    def close(self) -> None:
        self._capture.release()


def resolve_monitor_index(selector: str) -> int:
    with mss.mss() as capture:
        monitors = capture.monitors

    monitor_count = len(monitors) - 1
    if monitor_count <= 0:
        raise RuntimeError("No monitors found by mss.")

    normalized = (selector or "").strip().lower()
    if normalized in ("", "auto", "last", "virtual"):
        if monitor_count == 1:
            return 1
        return monitor_count

    if normalized in ("primary", "main"):
        return 1

    try:
        index = int(normalized)
    except ValueError as exc:
        raise RuntimeError(
            f"Invalid --monitor value '{selector}'. Use an index, 'auto', or 'primary'."
        ) from exc

    if index < 1 or index > monitor_count:
        raise RuntimeError(
            f"Monitor index {index} is out of range (valid: 1..{monitor_count})."
        )
    return index


def list_monitors() -> None:
    with mss.mss() as capture:
        monitors = capture.monitors
        if len(monitors) <= 1:
            print("No individual monitors were reported by mss.")
            return

        print("Detected monitors:")
        for idx, monitor in enumerate(monitors[1:], start=1):
            print(
                f"  {idx}: left={monitor['left']}, top={monitor['top']}, "
                f"width={monitor['width']}, height={monitor['height']}"
            )


def interpolation_for_resize(src_w: int, src_h: int, dst_w: int, dst_h: int) -> int:
    if dst_w < src_w or dst_h < src_h:
        return cv2.INTER_AREA
    return cv2.INTER_LINEAR


def resize_to_target(frame: np.ndarray, mode: str) -> np.ndarray:
    src_h, src_w = frame.shape[:2]
    if src_w <= 0 or src_h <= 0:
        raise RuntimeError("Input frame has invalid dimensions.")

    if mode == "fit":
        scale = min(TARGET_WIDTH / float(src_w), TARGET_HEIGHT / float(src_h))
        dst_w = max(1, int(round(src_w * scale)))
        dst_h = max(1, int(round(src_h * scale)))
        interp = interpolation_for_resize(src_w, src_h, dst_w, dst_h)
        resized = cv2.resize(frame, (dst_w, dst_h), interpolation=interp)

        output = np.zeros((TARGET_HEIGHT, TARGET_WIDTH, 3), dtype=np.uint8)
        x0 = (TARGET_WIDTH - dst_w) // 2
        y0 = (TARGET_HEIGHT - dst_h) // 2
        output[y0:y0 + dst_h, x0:x0 + dst_w] = resized
        return output

    scale = max(TARGET_WIDTH / float(src_w), TARGET_HEIGHT / float(src_h))
    dst_w = max(1, int(round(src_w * scale)))
    dst_h = max(1, int(round(src_h * scale)))
    interp = interpolation_for_resize(src_w, src_h, dst_w, dst_h)
    resized = cv2.resize(frame, (dst_w, dst_h), interpolation=interp)

    x0 = max(0, (dst_w - TARGET_WIDTH) // 2)
    y0 = max(0, (dst_h - TARGET_HEIGHT) // 2)
    cropped = resized[y0:y0 + TARGET_HEIGHT, x0:x0 + TARGET_WIDTH]

    if cropped.shape[1] != TARGET_WIDTH or cropped.shape[0] != TARGET_HEIGHT:
        return cv2.resize(cropped, (TARGET_WIDTH, TARGET_HEIGHT), interpolation=cv2.INTER_AREA)
    return cropped


def orient_frame(frame: np.ndarray, rotate: int, flip_h: bool, flip_v: bool) -> np.ndarray:
    if rotate == 90:
        frame = cv2.rotate(frame, cv2.ROTATE_90_CLOCKWISE)
    elif rotate == 180:
        frame = cv2.rotate(frame, cv2.ROTATE_180)
    elif rotate == 270:
        frame = cv2.rotate(frame, cv2.ROTATE_90_COUNTERCLOCKWISE)

    if flip_h:
        frame = cv2.flip(frame, 1)
    if flip_v:
        frame = cv2.flip(frame, 0)
    return frame


def bgr_to_rgb565_payload(frame: np.ndarray) -> bytes:
    blue = frame[:, :, 0].astype(np.uint16)
    green = frame[:, :, 1].astype(np.uint16)
    red = frame[:, :, 2].astype(np.uint16)

    packed = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
    payload = packed.astype("<u2").tobytes()

    if len(payload) != PAYLOAD_SIZE:
        raise RuntimeError(f"Invalid payload size {len(payload)} (expected {PAYLOAD_SIZE}).")
    return payload


def build_packet(payload: bytes, sequence: int, flags: int) -> bytes:
    header = struct.pack(
        "<2sBBHH",
        PROTO_MAGIC,
        PROTO_VERSION,
        flags & 0xFF,
        sequence & 0xFFFF,
        len(payload) & 0xFFFF,
    )
    return header + payload


def write_packet(port: serial.Serial, packet: bytes) -> None:
    total = 0
    view = memoryview(packet)
    while total < len(packet):
        written = port.write(view[total:])
        if written is None or written <= 0:
            raise serial.SerialTimeoutException("Serial write timeout.")
        total += written


def send_control_command(port: serial.Serial, command: int) -> None:
    payload = bytes([command & 0xFF])
    packet = build_packet(payload, 0, FLAG_CONTROL_PACKET)
    write_packet(port, packet)


def auto_detect_port() -> str:
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        raise RuntimeError("No serial ports found. Connect the Teensy or pass --port.")

    def score_port(port_info: serial.tools.list_ports_common.ListPortInfo) -> int:
        score = 0
        desc = (port_info.description or "").lower()
        hwid = (port_info.hwid or "").lower()
        manuf = (port_info.manufacturer or "").lower()

        if port_info.vid == 0x16C0:
            score += 40
        if port_info.pid == 0x0483:
            score += 40
        if "teensy" in desc or "teensy" in hwid or "teensy" in manuf:
            score += 20
        if "usb serial" in desc:
            score += 5
        return score

    ranked = sorted(ports, key=score_port, reverse=True)
    best_score = score_port(ranked[0])

    if best_score == 0 and len(ranked) > 1:
        choices = ", ".join([p.device for p in ranked])
        raise RuntimeError(
            "Could not auto-detect Teensy port from multiple serial ports. "
            f"Available: {choices}. Pass --port explicitly."
        )

    return ranked[0].device


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Stream a 64x32 RGB565 feed to ProtoTracer over USB serial."
    )
    parser.add_argument(
        "--port",
        help="Serial port (example: COM7 or /dev/ttyACM0). If omitted, auto-detection is used.",
    )
    parser.add_argument("--baud", type=int, default=2000000, help="Serial baud rate.")
    parser.add_argument("--fps", type=float, default=30.0, help="Target stream frame rate.")
    parser.add_argument(
        "--source",
        choices=("screen", "camera", "video"),
        default="screen",
        help="Capture source type.",
    )
    parser.add_argument(
        "--monitor",
        default="auto",
        help="Monitor index or selector for screen source: index, 'auto', or 'primary'.",
    )
    parser.add_argument(
        "--region",
        type=int,
        nargs=4,
        metavar=("LEFT", "TOP", "WIDTH", "HEIGHT"),
        help="Capture region for screen source.",
    )
    parser.add_argument("--camera-index", type=int, default=0, help="Camera index for camera source.")
    parser.add_argument("--video-path", help="Video file path when --source video is used.")
    parser.add_argument("--video-loop", action="store_true", help="Loop video when end is reached.")
    parser.add_argument(
        "--scale-mode",
        choices=("fill", "fit"),
        default="fill",
        help="fill=crop to fill panel, fit=letterbox/pillarbox.",
    )
    parser.add_argument("--rotate", type=int, choices=(0, 90, 180, 270), default=0)
    parser.add_argument("--flip-h", action="store_true", help="Flip frame horizontally.")
    parser.add_argument("--flip-v", action="store_true", help="Flip frame vertically.")
    parser.add_argument(
        "--single-panel",
        action="store_true",
        help="Disable lower-panel mirror (sends single-panel flag).",
    )
    parser.add_argument(
        "--force-mirror",
        action="store_true",
        help="Force lower-panel mirror flag.",
    )
    parser.add_argument("--preview", action="store_true", help="Show a local preview window.")
    parser.add_argument("--list-monitors", action="store_true", help="Print monitor list and exit.")
    return parser.parse_args()


def create_source(args: argparse.Namespace) -> FrameSource:
    if args.source == "screen":
        region_tuple: Optional[Tuple[int, int, int, int]] = None
        if args.region is not None:
            left, top, width, height = args.region
            if width <= 0 or height <= 0:
                raise RuntimeError("Region width and height must be greater than zero.")
            region_tuple = (left, top, width, height)
        monitor_index = resolve_monitor_index(args.monitor)
        return ScreenSource(monitor_index, region_tuple)

    if args.source == "camera":
        return VideoCaptureSource(args.camera_index, loop=False)

    if not args.video_path:
        raise RuntimeError("--video-path is required when --source video is selected.")
    return VideoCaptureSource(args.video_path, loop=args.video_loop)


def main() -> int:
    args = parse_args()

    if args.list_monitors:
        list_monitors()
        return 0

    if not args.port:
        args.port = auto_detect_port()
        print(f"Auto-detected serial port: {args.port}")
    if args.source != "screen" and args.region is not None:
        raise RuntimeError("--region can only be used with --source screen.")
    if args.source != "video" and args.video_path:
        raise RuntimeError("--video-path can only be used with --source video.")
    if args.fps <= 0.0:
        raise RuntimeError("--fps must be greater than zero.")

    flags = 0
    if args.force_mirror:
        flags |= FLAG_MIRROR_LOWER
    if args.single_panel:
        flags |= FLAG_SINGLE_PANEL

    source = create_source(args)
    port: Optional[serial.Serial] = None

    try:
        port = serial.Serial(args.port, args.baud, timeout=0, write_timeout=1.0)
        time.sleep(0.2)
        port.reset_input_buffer()
        port.reset_output_buffer()
        send_control_command(port, CMD_START_STREAM_MODE)
        time.sleep(0.03)

        frame_interval = 1.0 / float(args.fps)
        next_frame_time = time.perf_counter()
        sequence = 0

        stat_start = time.perf_counter()
        sent_frames = 0

        print(
            f"Streaming to {args.port} at {args.baud} baud | source={args.source} | "
            f"target={TARGET_WIDTH}x{TARGET_HEIGHT} | fps={args.fps:.1f}"
        )
        print("Press Ctrl+C to stop.")

        while True:
            now = time.perf_counter()
            if now < next_frame_time:
                time.sleep(min(0.001, next_frame_time - now))
                continue

            frame = source.read()
            if frame is None:
                if args.source == "video" and not args.video_loop:
                    print("\nEnd of video stream.")
                    break
                next_frame_time = now + frame_interval
                continue

            frame = orient_frame(frame, args.rotate, args.flip_h, args.flip_v)
            frame = resize_to_target(frame, args.scale_mode)

            payload = bgr_to_rgb565_payload(frame)
            packet = build_packet(payload, sequence, flags)
            write_packet(port, packet)

            sequence = (sequence + 1) & 0xFFFF
            sent_frames += 1

            if args.preview:
                preview = cv2.resize(
                    frame,
                    (TARGET_WIDTH * 8, TARGET_HEIGHT * 8),
                    interpolation=cv2.INTER_NEAREST,
                )
                cv2.imshow("ProtoTracer USB Stream", preview)
                key = cv2.waitKey(1) & 0xFF
                if key in (27, ord("q")):
                    print("\nPreview closed by user.")
                    break

            next_frame_time += frame_interval
            if (now - next_frame_time) > frame_interval:
                next_frame_time = now + frame_interval

            elapsed = now - stat_start
            if elapsed >= 1.0:
                fps = sent_frames / elapsed
                print(f"\rTX FPS: {fps:5.1f}", end="", flush=True)
                stat_start = now
                sent_frames = 0

        print("\nStopped.")
        return 0

    finally:
        if port is not None and port.is_open:
            try:
                send_control_command(port, CMD_STOP_STREAM_MODE)
            except Exception:
                pass
        source.close()
        if args.preview:
            cv2.destroyAllWindows()
        if port is not None and port.is_open:
            port.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nStopped.")
        raise SystemExit(0)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)
