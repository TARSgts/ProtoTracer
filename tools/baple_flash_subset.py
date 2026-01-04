#!/usr/bin/env python3
import argparse
from pathlib import Path


HEADER_PREFIX = """#pragma once

#include "Arduino.h"
#include "../../../Scene/Materials/Material.h"
#include "../../../Utils/Math/Vector2D.h"
#include "../../../Utils/Math/Mathematics.h"

class BapleFlash300Sequence : public Material {
private:
    static const uint8_t* const sequence[@@FRAME_COUNT@@];
@@FRAME_DECLS@@

    Vector2D size;
    Vector2D offset;
    float angle = 0.0f;

    unsigned long startTime = 0;
    unsigned int imageCount = @@FRAME_COUNT@@;
    float fps = @@FPS@@;
    float frameTime = 0.0f;
    unsigned int currentFrame = 0;

    static constexpr unsigned int kWidth = 64;
    static constexpr unsigned int kHeight = 32;

public:
    BapleFlash300Sequence(Vector2D size, Vector2D offset, float fps)
        : size(size), offset(offset), fps(fps) {
        SetFPS(fps);
    }

    void SetActive(bool enabled) {
        (void)enabled;
    }

    void SetFPS(float fps) {
        this->fps = fps;
        frameTime = fps > 0.0f ? (1000.0f / fps) : 0.0f;
    }

    void SetSize(Vector2D size) {
        this->size = size;
    }

    void SetPosition(Vector2D offset) {
        this->offset = offset;
    }

    void SetRotation(float angle) {
        this->angle = angle;
    }

    void Reset() {
        startTime = millis();
        currentFrame = 0;
    }

    void Update() {
        if (frameTime <= 0.0f) return;
        if (startTime == 0) startTime = millis();
        unsigned long elapsed = millis() - startTime;
        currentFrame = static_cast<unsigned int>((elapsed / frameTime)) % imageCount;
    }

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override {
        (void)normal;
        (void)uvw;
        Vector2D rPos = angle != 0.0f ? Vector2D(position.X, position.Y).Rotate(angle, offset) - offset
                                        : Vector2D(position.X, position.Y) - offset;

        unsigned int x = (unsigned int)Mathematics::Map(rPos.X, size.X / -2.0f, size.X / 2.0f, float(kWidth), 0.0f);
        unsigned int y = (unsigned int)Mathematics::Map(rPos.Y, size.Y / -2.0f, size.Y / 2.0f, float(kHeight), 0.0f);

        if (x <= 1 || x >= kWidth || y <= 1 || y >= kHeight) return RGBColor();

        uint32_t index = x + y * kWidth;
        const uint8_t* frame = reinterpret_cast<const uint8_t*>(pgm_read_ptr(&sequence[currentFrame]));
        uint8_t byteValue = pgm_read_byte(frame + (index >> 3));
        bool bit = (byteValue >> (7 - (index & 7))) & 0x1;
        return bit ? RGBColor(255, 255, 255) : RGBColor(0, 0, 0);
    }
};

"""


def chunk_bytes(data: bytes, per_line: int = 16) -> str:
    parts = []
    for i in range(0, len(data), per_line):
        slice_ = data[i : i + per_line]
        parts.append(",".join(str(b) for b in slice_))
    return ",".join(parts)


def select_indices(total: int, frames: int, mode: str) -> list[int]:
    if frames <= 0:
        return []
    if mode == "first":
        return list(range(min(frames, total)))
    if frames == 1:
        return [0]
    indices = []
    for i in range(frames):
        idx = int(round(i * (total - 1) / (frames - 1)))
        indices.append(idx)
    return indices


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate flash header from BAPLE.BIN.")
    parser.add_argument("input", type=Path, help="Input BAPLE.BIN file.")
    parser.add_argument("output", type=Path, help="Output header path.")
    parser.add_argument("--frames", type=int, default=300, help="Number of frames to include.")
    parser.add_argument("--frame-bytes", type=int, default=256, help="Bytes per frame.")
    parser.add_argument("--fps", type=float, default=18.0, help="Playback FPS.")
    parser.add_argument("--mode", choices=("even", "first"), default="even", help="Frame selection mode.")
    args = parser.parse_args()

    raw = args.input.read_bytes()
    if len(raw) % args.frame_bytes != 0:
        raise SystemExit(f"Input size {len(raw)} not divisible by {args.frame_bytes}")
    total_frames = len(raw) // args.frame_bytes
    indices = select_indices(total_frames, args.frames, args.mode)
    frame_count = len(indices)

    frame_decls = []
    for i in range(frame_count):
        frame_decls.append(f"    static const uint8_t frame{i:04d}[];")
    decl_block = "\n".join(frame_decls)

    header = HEADER_PREFIX.replace("@@FRAME_COUNT@@", str(frame_count))
    header = header.replace("@@FRAME_DECLS@@", decl_block)
    header = header.replace("@@FPS@@", f"{args.fps:.1f}")

    out = [header]

    for i, idx in enumerate(indices):
        start = idx * args.frame_bytes
        end = start + args.frame_bytes
        data = raw[start:end]
        out.append(f"inline const uint8_t BapleFlash300Sequence::frame{i:04d}[] PROGMEM = {{{chunk_bytes(data)}}};\n")

    seq_items = ",".join(f"frame{i:04d}" for i in range(frame_count))
    out.append(
        f"inline const uint8_t* const BapleFlash300Sequence::sequence[{frame_count}] PROGMEM = {{{seq_items}}};\n"
    )

    args.output.write_text("".join(out), encoding="ascii")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
