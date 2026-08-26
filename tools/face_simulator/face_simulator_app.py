#!/usr/bin/env python3
"""ProtoTracer Face Simulator.

Previews the pixel logic of "auto-play" Material faces (the kind wired up in
ProtogenProjectTemplate + ENABLE_*_FACE, see lib/ProtoTracer/Scene/Materials/Animated)
on the desktop, without flashing hardware. Each face is reimplemented in pure Python,
matching the C++ Update()/GetRGB() logic constant-for-constant.

Controls:
    Hold SPACE (or the BOOP button) to simulate the boop sensor / action input.
    R resets the current face.

Run:
    python tools/face_simulator/face_simulator_app.py
"""

from __future__ import annotations

import math
import random
import time
import tkinter as tk
from tkinter import ttk

# Compact 3x5 pixel digit font. The firmware score HUD originally used the shared
# TextEngine class (also used by Menu/Clock), but its internal Y-axis Map() has an
# off-by-one at the box boundary (position mapping to exactly lineCount*10 fails the
# `< lineCount*10` bounds check and renders black) which clipped a row of the score --
# switched both this preview and the real DinoGame.cpp to this same hand-drawn digit
# bitmap instead, matching the dino/cactus bitmap pattern (see memory notes).
SCORE_FONT = {
    "0": ["111", "101", "101", "101", "111"],
    "1": ["010", "110", "010", "010", "111"],
    "2": ["111", "001", "111", "100", "111"],
    "3": ["111", "001", "111", "001", "111"],
    "4": ["101", "101", "111", "001", "001"],
    "5": ["111", "100", "111", "001", "111"],
    "6": ["111", "100", "111", "101", "111"],
    "7": ["111", "001", "001", "001", "001"],
    "8": ["111", "101", "111", "101", "111"],
    "9": ["111", "101", "111", "001", "111"],
}

# Each entry: (logical_w, logical_h, physical_w, physical_h). The face's game logic
# (dino/obstacle sizing, physics) runs in the LOGICAL coordinate space -- that's what
# ProtogenProjectTemplate.cpp passes as the camera size. But a Material's GetRGB() is
# only ever sampled once per PHYSICAL LED, and the physical LED count is far smaller
# than the logical canvas suggests:
#   lib/ProtoTracer/Camera/CameraManager/Implementations/HUB75DeltaCameras.h:
#     PixelGroup<2048> camPixels(Vector2D(192, 96), Vector2D(0,0), 64);  // 2048 = 64*32
# i.e. HUB75 is a 64x32 physical grid covering that 192x96 logical area -- a pixel pitch
# of 3 logical units per physical LED. Rendering the logical canvas directly (as earlier
# versions of this tool did) is misleading: fine detail that looks fine at 192x94 turns
# into sub-pixel mush or flicker once actually sampled at 64x32. Always render at the
# physical resolution to see what the hardware will really show.
CANVAS_SIZES = {
    "HUB75 (64x32 physical)": (192, 94, 64, 32),
    "WS35 (approx, irregular layout)": (192, 105, 64, 35),
}

FRAME_MS = 33  # ~30 FPS preview loop.


def clampf(value: float, low: float, high: float) -> float:
    return low if value < low else (high if value > high else value)


def fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, x0, x1, y0, y1, hex_color) -> None:
    """Fills a material-space rectangle (centered coords, Y-up) into a pixel buffer (Y-down
    rows). `canvas_w`/`canvas_h` may be a coarser physical-pixel grid than `half_w`/`half_h`
    (logical half-extents) imply -- the pitch (logical units per buffer cell) is derived
    from the two so this works whether buffer is at logical or physical resolution."""
    if x1 <= x0 or y1 <= y0:
        return
    pitch_x = (2.0 * half_w) / canvas_w
    pitch_y = (2.0 * half_h) / canvas_h
    col0 = max(0, int(math.floor((x0 + half_w) / pitch_x)))
    col1 = min(canvas_w, int(math.ceil((x1 + half_w) / pitch_x)))
    row0 = max(0, int(math.floor((half_h - y1) / pitch_y)))
    row1 = min(canvas_h, int(math.ceil((half_h - y0) / pitch_y)))
    if col1 <= col0 or row1 <= row0:
        return
    fill = [hex_color] * (col1 - col0)
    for row in range(row0, row1):
        buffer[row][col0:col1] = fill


def to_hex(rgb) -> str:
    r, g, b = rgb
    return f"#{r:02x}{g:02x}{b:02x}"


class DinoGameFace:
    """Python port of DinoGameMaterial (lib/ProtoTracer/Scene/Materials/Animated/DinoGame.cpp).

    Kept constant-for-constant with the firmware version. The boop sensor is fed in via
    set_action_pressed(), matching ProtogenProject::DinoAutoFace() calling
    dino.SetJumpPressed(IsBooped()). Cacti come in 3 fixed-size variants (small/medium/
    double), each verified against the dino's actual jump apex height so every variant
    is clearable with margin -- "double" (two small cacti side by side) is additionally
    gated to only spawn once the game has sped up enough (K_DOUBLE_MIN_SPEED), since its
    combined width isn't safely clearable at the game's starting speed.
    """

    name = "Dino Game (boop = jump)"
    action_label = "BOOP (jump)"
    OBSTACLE_COUNT = 3
    CLOUD_COUNT = 3

    # Hand-drawn pixel-art sprite (top row first), not derived from proportional
    # rectangle math -- three rounds of "cut a fraction of the body away" produced an
    # unrecognizable blob at ~6-10 physical pixels. Each character here is exactly one
    # physical LED (see PIXEL_PITCH). '.'=background, 'X'=body, 'E'=eye.
    # Body is shared across poses; only the two feet rows change, and only by shifting
    # position (never a full row going blank) -- alternating a large filled/empty
    # region every frame is what caused the earlier "flickering" complaint at this
    # resolution. A small shift reads as a step; a vanish reads as noise.
    DINO_BODY = [
        "......XX.",
        ".....XXXX",
        ".....XEXX",
        "....XXXXX",
        "...XXXXX.",
        "..XXXXXX.",
        ".XXXXXXX.",
        ".XXXXXX..",
        "XXXXXX...",
    ]
    # Feet alternate between two hand-drawn frames that both keep the legs populated --
    # shifting position, not blacking out a big region -- since a full vanish/reappear
    # read as flicker rather than running at this resolution (an earlier attempt at
    # dropping animation entirely to chase down a separate flicker report turned out to
    # remove a wanted feature rather than fix the actual bug -- keep this animated).
    DINO_FEET_A = [
        ".XX..XX..",
        ".X....X..",
    ]
    DINO_FEET_B = [
        "..XXXX...",
        "...XX....",
    ]
    K_LEG_SWAP_SECONDS = 0.18
    # Logical units per physical LED -- matches HUB75's real pitch (192 logical units /
    # 64 physical LEDs, see HUB75DeltaCameras.h) so each bitmap character is exactly one
    # LED, not a fraction of one. Sized in fixed physical pixels rather than as a
    # fraction of canvas size, like a real sprite would be. Shared by the dino, cactus,
    # and score-digit bitmaps so everything sits on the same physical grid.
    PIXEL_PITCH = 3.0

    # Three hand-drawn cactus variants for visual variety (real Chrome Dino similarly
    # has small/large sprites plus small clusters) -- replacing the single fixed cactus,
    # which read as "all the same" despite being safely sized. Visual footprint is
    # taller/wider than the actual gameplay hitbox in each case (top rows are decorative
    # upward-curving arm tips) -- same "visual bigger than hitbox" trick as the dino's
    # width. Collision dims are independently verified against the jump apex below.
    CACTUS_SMALL_BITMAP = [
        "X...X",
        "X...X",
        "X.X.X",
        ".XXX.",
        "..X..",
        "..X..",
        "..X..",
    ]
    CACTUS_SMALL_COLLISION_WIDTH = 9.0    # 3 physical px.
    CACTUS_SMALL_COLLISION_HEIGHT = 15.0  # 5 physical px.

    # Taller trunk, still only as wide as SMALL -- the extra height is what makes it
    # read as "medium", not extra bulk.
    CACTUS_MEDIUM_BITMAP = [
        "X...X",
        "X...X",
        "X...X",
        "X.X.X",
        ".XXX.",
        "..X..",
        "..X..",
        "..X..",
        "..X..",
    ]
    # Collision height intentionally matches SMALL exactly (not the taller visual) --
    # Medium being "still too hard to clear" persisted across multiple margin increases
    # because a taller hitbox was directly fighting the apex-based fairness fix. Rather
    # than keep raising the jump apex to compensate for one obstacle's height, decouple
    # it: Medium is purely a visual variant (taller sprite) with the exact same,
    # already-comfortable difficulty as SMALL.
    CACTUS_MEDIUM_COLLISION_WIDTH = 9.0
    CACTUS_MEDIUM_COLLISION_HEIGHT = 15.0

    # Two SMALL cacti side by side, as one obstacle -- built from CACTUS_SMALL_BITMAP
    # with a 1-column gap rather than hand-typed, so every row is guaranteed the same
    # length (a hand-typed first draft had inconsistent gap widths between rows, which
    # would have misaligned the bitmap). Combined width roughly doubles the overlap
    # window the dino must clear, which only has a safe margin once the game has sped
    # up -- see K_DOUBLE_MIN_SPEED, checked before this variant is ever chosen.
    CACTUS_DOUBLE_BITMAP = [row + "." + row for row in CACTUS_SMALL_BITMAP]
    CACTUS_DOUBLE_COLLISION_WIDTH = 21.0  # 2x small collision width + 1 physical px gap.
    CACTUS_DOUBLE_COLLISION_HEIGHT = 15.0
    K_DOUBLE_MIN_SPEED = 70.0  # verified via overlap-window-vs-high-enough-duration below.

    K_MAX_DELTA = 0.05
    K_HITBOX_SHRINK = 0.82
    # The drawn sprite is sized for legibility, not fairness -- the gameplay hitbox is
    # narrower than the visual body (see _check_collision). Widening the hitbox to match
    # the full (bigger, post-bitmap-redesign) sprite made tall/wide obstacles literally
    # uncrossable at low speed: verified numerically that the required "stay above
    # obstacle height" window exceeded the time available above that height.
    K_COLLISION_WIDTH_FRACTION = 0.6
    # Real Chrome Dino takes ~117s to ramp SPEED(6) to MAX_SPEED(13) at
    # ACCELERATION=0.001/frame@60fps -- i.e. very gradual. Match that order of magnitude
    # instead of a fast ramp that outpaces the player's reaction time.
    K_SPEED_RAMP_SECONDS = 120.0
    # Real game sizes the gap between obstacles off current speed (minGap = width*speed
    # + typeConfig.minGap*GAP_COEFFICIENT, maxGap = minGap*1.5) specifically so the
    # *time* to cross a gap stays roughly constant as speed ramps up. We do the same:
    # gap is speed*GAP_SECONDS (+0-50% extra), not a fixed pixel distance.
    K_GAP_SECONDS = 1.8

    def __init__(self, width: float, height: float):
        # Monochrome palette, matching the real Chrome Dino's black/white look (this is
        # the "night mode" polarity -- black background, white foreground -- since every
        # other face in this project uses a black background too). Dino/cactus are pure
        # white ("hero" foreground); ground is a clearly dimmer mid-gray so it reads as
        # the floor rather than blending into the sprites standing on it; hills/clouds
        # are dimmer still, for background depth.
        self.sky_hex = to_hex((0, 0, 0))
        self.ground_rgb = (150, 150, 150)
        self.hill_rgb = (55, 55, 55)
        self.ground_hex = to_hex(self.ground_rgb)
        self.hill_hex = to_hex(self.hill_rgb)
        self.cloud_hex = to_hex((95, 95, 95))
        self.dino_hex = to_hex((255, 255, 255))
        self.dino_eye_hex = to_hex((0, 0, 0))
        self.cactus_hex = to_hex((255, 255, 255))
        self.score_rgb = (255, 255, 255)

        self.on_ground = True
        self.jump_latched = False
        self.leg_phase = False
        self.leg_timer = 0.0
        self.dino_x = 0.0
        self.dino_y = 0.0
        self.dino_velocity = 0.0
        self.survival_time = 0.0
        self.hill_phase = 0.0
        self.distance = 0.0
        self.score = 0

        self.obstacle_x = [0.0] * self.OBSTACLE_COUNT
        self.obstacle_type = ["small"] * self.OBSTACLE_COUNT

        self.cloud_x = [0.0] * self.CLOUD_COUNT
        self.cloud_y = [0.0] * self.CLOUD_COUNT

        self.set_size(width, height)

    def set_size(self, width: float, height: float) -> None:
        self.half_w = width / 2.0
        self.half_h = height / 2.0
        self._recalculate_dimensions()
        self._reset_clouds()
        self.reset()

    def _recalculate_dimensions(self) -> None:
        self.ground_height = max(2.0, self.half_h * 0.08)
        self.dino_width = len(self.DINO_BODY[0]) * self.PIXEL_PITCH
        self.dino_height = (len(self.DINO_BODY) + len(self.DINO_FEET_A)) * self.PIXEL_PITCH
        self.min_obstacle_speed = max(35.0, self.half_w * 0.5)
        self.max_obstacle_speed = self.min_obstacle_speed * 2.2
        self.gravity = -max(150.0, self.half_h * 4.5)
        self.jump_velocity = max(90.0, self.half_h * 2.35)
        # Peak jump height reachable with the above gravity/velocity; obstacle heights
        # are sized as a fraction of this so every obstacle is always jumpable.
        self.apex_height = (self.jump_velocity ** 2) / (2.0 * abs(self.gravity))

        self.speed_ramp_per_second = (self.max_obstacle_speed - self.min_obstacle_speed) / self.K_SPEED_RAMP_SECONDS
        self.min_gap_pixels = self.half_w * 0.35

        # Hills: a prominent background dune/mountain line (peak reaches ~38% of half-height).
        self.hill_base_height = max(4.0, self.half_h * 0.14)
        self.hill_amplitude = max(6.0, self.half_h * 0.24)
        self.hill_speed = self.min_obstacle_speed * 0.35
        self.hill_frequency = 3.5 / max(1.0, self.half_w)

        # Clouds: big, wide puffs, drifting slowly.
        self.cloud_width = max(20.0, self.half_w * 0.30)
        self.cloud_height = self.cloud_width * 0.55
        self.cloud_speed = self.min_obstacle_speed * 0.18

    def _compute_gap(self) -> float:
        base = max(self.min_gap_pixels, self.obstacle_speed * self.K_GAP_SECONDS)
        return base + base * random.uniform(0.0, 0.5)

    def _obstacle_bitmap(self, obstacle_type: str):
        return {
            "small": self.CACTUS_SMALL_BITMAP,
            "medium": self.CACTUS_MEDIUM_BITMAP,
            "double": self.CACTUS_DOUBLE_BITMAP,
        }[obstacle_type]

    def _obstacle_collision_size(self, obstacle_type: str):
        return {
            "small": (self.CACTUS_SMALL_COLLISION_WIDTH, self.CACTUS_SMALL_COLLISION_HEIGHT),
            "medium": (self.CACTUS_MEDIUM_COLLISION_WIDTH, self.CACTUS_MEDIUM_COLLISION_HEIGHT),
            "double": (self.CACTUS_DOUBLE_COLLISION_WIDTH, self.CACTUS_DOUBLE_COLLISION_HEIGHT),
        }[obstacle_type]

    def _random_obstacle_type(self) -> str:
        # "double" (two small cacti side by side) roughly doubles the horizontal span
        # the dino must clear in one jump -- only safe once the game has sped up enough
        # (verified: overlap-window-vs-high-enough-duration goes negative below this).
        choices = ["small", "medium"]
        if self.obstacle_speed >= self.K_DOUBLE_MIN_SPEED:
            choices.append("double")
        return random.choice(choices)

    def _reset_obstacle(self, index: int, x_position: float) -> None:
        self.obstacle_x[index] = x_position
        self.obstacle_type[index] = self._random_obstacle_type()

    def _sky_band(self):
        sky_top = self.half_h
        sky_bottom = -self.half_h + self.ground_height + self.hill_base_height + self.hill_amplitude + self.cloud_height
        return sky_bottom, max(1.0, sky_top - sky_bottom)

    def _reset_clouds(self) -> None:
        sky_bottom, span = self._sky_band()
        for i in range(self.CLOUD_COUNT):
            self.cloud_x[i] = random.uniform(-self.half_w, self.half_w)
            self.cloud_y[i] = sky_bottom + random.uniform(0.0, span)

    def _update_clouds(self, dt: float) -> None:
        sky_bottom, span = self._sky_band()
        for i in range(self.CLOUD_COUNT):
            self.cloud_x[i] -= self.cloud_speed * dt
            if self.cloud_x[i] < -self.half_w - self.cloud_width:
                self.cloud_x[i] = self.half_w + self.cloud_width
                self.cloud_y[i] = sky_bottom + random.uniform(0.0, span)

    def reset(self) -> None:
        self.dino_x = -self.half_w * 0.6
        self.dino_y = 0.0
        self.dino_velocity = 0.0
        self.on_ground = True
        self.leg_phase = False
        self.leg_timer = 0.0
        self.survival_time = 0.0
        self.distance = 0.0
        self.score = 0
        self.obstacle_speed = self.min_obstacle_speed

        x = self.half_w + self._compute_gap() * 0.5
        for i in range(self.OBSTACLE_COUNT):
            self._reset_obstacle(i, x)
            x += self._compute_gap()

    def set_action_pressed(self, pressed: bool) -> None:
        if pressed and not self.jump_latched and self.on_ground:
            self.dino_velocity = self.jump_velocity
            self.on_ground = False
        self.jump_latched = pressed

    def _check_collision(self) -> bool:
        half_width = self.dino_width * 0.5 * self.K_HITBOX_SHRINK * self.K_COLLISION_WIDTH_FRACTION
        dino_bottom = -self.half_h + self.ground_height + self.dino_y
        dino_top = dino_bottom + self.dino_height * self.K_HITBOX_SHRINK
        dino_left = self.dino_x - half_width
        dino_right = self.dino_x + half_width

        for i in range(self.OBSTACLE_COUNT):
            cw, ch = self._obstacle_collision_size(self.obstacle_type[i])
            left = self.obstacle_x[i] - cw * 0.5
            right = self.obstacle_x[i] + cw * 0.5
            if dino_right < left or dino_left > right:
                continue
            obstacle_bottom = -self.half_h + self.ground_height
            obstacle_top = obstacle_bottom + ch
            if dino_top > obstacle_bottom and dino_bottom < obstacle_top:
                return True
        return False

    def update(self, dt: float) -> None:
        dt = min(dt, self.K_MAX_DELTA)

        self.survival_time += dt
        self.obstacle_speed = min(
            self.max_obstacle_speed,
            self.min_obstacle_speed + self.survival_time * self.speed_ramp_per_second,
        )
        self.distance += self.obstacle_speed * dt
        self.score = int(self.distance)

        if not self.on_ground:
            self.dino_velocity += self.gravity * dt
            self.dino_y += self.dino_velocity * dt
            if self.dino_y <= 0.0:
                self.dino_y = 0.0
                self.dino_velocity = 0.0
                self.on_ground = True
        else:
            self.leg_timer += dt
            if self.leg_timer >= self.K_LEG_SWAP_SECONDS:
                self.leg_timer = 0.0
                self.leg_phase = not self.leg_phase

        max_x = self.obstacle_x[0]
        for i in range(self.OBSTACLE_COUNT):
            self.obstacle_x[i] -= self.obstacle_speed * dt
            if self.obstacle_x[i] > max_x:
                max_x = self.obstacle_x[i]

        # Widest possible visual footprint (double) is used for the off-screen check so
        # a wide cluster doesn't visually pop out of existence before it's fully gone.
        cactus_visual_width = len(self.CACTUS_DOUBLE_BITMAP[0]) * self.PIXEL_PITCH
        for i in range(self.OBSTACLE_COUNT):
            if self.obstacle_x[i] < -self.half_w - cactus_visual_width:
                max_x += self._compute_gap()
                self._reset_obstacle(i, max_x)

        self._update_clouds(dt)
        hill_period = (2.0 * math.pi) / max(0.0001, self.hill_frequency)
        self.hill_phase = math.fmod(self.hill_phase + self.hill_speed * dt, hill_period)

        if self._check_collision():
            self.reset()

    def render(self, buffer, canvas_w: int, canvas_h: int) -> None:
        half_w, half_h = self.half_w, self.half_h
        sky = self.sky_hex
        for row in buffer:
            for i in range(canvas_w):
                row[i] = sky

        # Clouds first (furthest back): a wide flat base plus three overlapping puff
        # lobes on top, so the silhouette reads as rounded rather than a plain box.
        for i in range(self.CLOUD_COUNT):
            cx, cy = self.cloud_x[i], self.cloud_y[i]
            w, h = self.cloud_width, self.cloud_height
            fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, cx - w * 0.5, cx + w * 0.5, cy - h * 0.35, cy + h * 0.05, self.cloud_hex)
            fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, cx - w * 0.36, cx - w * 0.02, cy - h * 0.05, cy + h * 0.32, self.cloud_hex)
            fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, cx - w * 0.20, cx + w * 0.20, cy + h * 0.05, cy + h * 0.48, self.cloud_hex)
            fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, cx + w * 0.02, cx + w * 0.38, cy - h * 0.08, cy + h * 0.30, self.cloud_hex)

        pitch_x = (2.0 * half_w) / canvas_w
        pitch_y = (2.0 * half_h) / canvas_h

        ground_y = -half_h + self.ground_height
        ground_row_start = max(0, int(math.floor((half_h - ground_y) / pitch_y)))
        for col in range(canvas_w):
            material_x = (col + 0.5) * pitch_x - half_w
            wave = math.sin((material_x + self.hill_phase) * self.hill_frequency) * 0.5 + 0.5
            hill_top_y = ground_y + self.hill_base_height + self.hill_amplitude * wave
            row_top = max(0, int(math.floor((half_h - hill_top_y) / pitch_y)))
            row_bottom = min(canvas_h, ground_row_start)
            for row in range(row_top, row_bottom):
                buffer[row][col] = self.hill_hex

        # Ground sits in front of the hills.
        for y in range(ground_row_start, canvas_h):
            buffer[y] = [self.ground_hex] * canvas_w

        # Ground edge anti-aliasing: the horizon line is horizontal (no x-dependence, and
        # hills always reach the ground line here since hill_base_height comfortably
        # exceeds one physical pixel), so instead of a per-pixel loop this blends just the
        # one or two rows straddling ground_y -- same 4-sample coverage technique as the
        # score HUD and the C++ side, applied as a post-hoc row overwrite (same pattern
        # already used for the forced-black bottom row above).
        for row in (ground_row_start - 1, ground_row_start):
            if row < 0 or row >= canvas_h:
                continue
            world_y = half_h - (row + 0.5) * pitch_y
            coverage = 0.0
            for dy in (-0.25, 0.25, -0.25, 0.25):
                if world_y + dy * self.PIXEL_PITCH < ground_y:
                    coverage += 0.25
            if 0.0 < coverage < 1.0:
                blended = tuple(int(h + (g - h) * coverage) for h, g in zip(self.hill_rgb, self.ground_rgb))
                buffer[row] = [to_hex(blended)] * canvas_w

        # Obstacles: cacti, hand-drawn pixel-art bitmaps, same technique as the dino.
        # Each obstacle picked its variant (small/medium/double) in _reset_obstacle, so
        # there's real visual variety instead of every cactus looking identical.
        for i in range(self.OBSTACLE_COUNT):
            bitmap = self._obstacle_bitmap(self.obstacle_type[i])
            cactus_rows = len(bitmap)
            cactus_cols = len(bitmap[0])
            cactus_left = self.obstacle_x[i] - (cactus_cols * self.PIXEL_PITCH) * 0.5
            for r, row_bits in enumerate(bitmap):
                y0 = ground_y + (cactus_rows - 1 - r) * self.PIXEL_PITCH
                y1 = y0 + self.PIXEL_PITCH
                for c, cell in enumerate(row_bits):
                    if cell == ".":
                        continue
                    x0 = cactus_left + c * self.PIXEL_PITCH
                    x1 = x0 + self.PIXEL_PITCH
                    fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, x0, x1, y0, y1, self.cactus_hex)

        # Dino: hand-drawn pixel-art bitmap, blitted one physical LED at a time. Feet
        # alternate between two frames that both keep the legs populated -- a shift, not
        # a vanish -- see DINO_FEET_A/B comment.
        feet = self.DINO_FEET_A if self.leg_phase else self.DINO_FEET_B
        dino_bitmap = self.DINO_BODY + feet

        dino_bottom = ground_y + self.dino_y
        dino_left = self.dino_x - self.dino_width * 0.5
        pitch = self.PIXEL_PITCH
        rows = len(dino_bitmap)
        for r, row_bits in enumerate(dino_bitmap):
            y0 = dino_bottom + (rows - 1 - r) * pitch
            y1 = y0 + pitch
            for c, cell in enumerate(row_bits):
                if cell == ".":
                    continue
                x0 = dino_left + c * pitch
                x1 = x0 + pitch
                color = self.dino_eye_hex if cell == "E" else self.dino_hex
                fill_rect(buffer, canvas_w, canvas_h, half_w, half_h, x0, x1, y0, y1, color)

        self._draw_score(buffer, canvas_w, canvas_h)

        # Force the bottom-most physical row to black. Reported as a persistent flicker
        # at the very bottom edge of the display -- suspected cause is a boundary
        # instability in how the shared camera/rasterizer samples the last physical row
        # (the PixelGroup for HUB75 is declared as a 96-logical-unit-tall area but the
        # face canvas only uses 94 of them, see HUB75DeltaCameras.h) rather than
        # anything specific to this face's own rendering logic. This is a direct,
        # unconditional workaround requested in place of chasing that root cause.
        buffer[canvas_h - 1] = [self.sky_hex] * canvas_w

    # Each SCORE_FONT pixel is drawn as a SCALExSCALE block of physical LEDs rather than
    # a single one -- at 1:1 the 3x5 digits were reported "completely illegible" on real
    # hardware (thin 1-pixel strokes wash out under LED diffusion/blur), but 2x ("way too
    # big") ate over 60% of the screen width. 1.5 is a middle ground -- drawn via
    # continuous floor/ceil math (like fill_rect) rather than literal NxN block
    # replication, since 1.5 isn't an integer repeat count.
    SCORE_SCALE = 1.5

    def _draw_score(self, buffer, canvas_w: int, canvas_h: int) -> None:
        """Top-left score HUD, drawn directly in physical-pixel space (see SCORE_FONT).
        Anti-aliased via 4x supersampling + coverage blending -- the same technique
        TextEngine.tpp uses internally -- mirrored here from DinoGame.cpp's GetRGB() so
        the score gets smoothed edges while the dino/cactus/ground stay crisp blocky
        pixel art. Each output physical pixel takes 4 sub-samples at quarter-pixel
        offsets; coverage (0, 0.25, ..., 1.0) is blended between black and score_rgb."""
        digits = str(self.score).zfill(5)
        margin = 1
        scale = self.SCORE_SCALE
        cell_cols = 3 + 1  # digit width + 1-cell gap, in font-cell units
        score_w = 5 * cell_cols * scale
        score_h = 5 * scale
        if margin + score_w > canvas_w:
            return

        sample_offsets = ((-0.25, -0.25), (0.25, -0.25), (-0.25, 0.25), (0.25, 0.25))
        py0 = max(0, margin)
        py1 = min(canvas_h, int(math.ceil(margin + score_h)))
        px0 = max(0, margin)
        px1 = min(canvas_w, int(math.ceil(margin + score_w)))

        for py in range(py0, py1):
            for px in range(px0, px1):
                coverage = 0.0
                for dx, dy in sample_offsets:
                    sx = (px + 0.5 + dx) - margin
                    sy = (py + 0.5 + dy) - margin
                    if sx < 0.0 or sx >= score_w or sy < 0.0 or sy >= score_h:
                        continue
                    total_col = int(math.floor(sx / scale))
                    row = int(math.floor(sy / scale))
                    digit_index = total_col // cell_cols
                    col_in_digit = total_col % cell_cols
                    if digit_index >= 5 or col_in_digit >= 3:
                        continue
                    if SCORE_FONT[digits[digit_index]][row][col_in_digit] == "1":
                        coverage += 0.25
                if coverage > 0.0:
                    buffer[py][px] = to_hex(tuple(int(c * coverage) for c in self.score_rgb))

    def status_text(self) -> str:
        state = "on ground" if self.on_ground else "airborne"
        return (
            f"score: {self.score:05d}   "
            f"speed: {self.obstacle_speed:5.1f} px/s   "
            f"apex: {self.apex_height:4.1f}   "
            f"dino: {state}"
        )


# Registry of previewable faces. Add a new entry here once a face class exposes
# __init__(width, height), set_size(width, height), reset(), set_action_pressed(bool),
# update(dt), render(buffer, w, h), and status_text() -> str.
FACE_REGISTRY = {
    DinoGameFace.name: DinoGameFace,
}


class FaceSimulatorApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("ProtoTracer Face Simulator")
        self.resizable(False, False)

        self.scale = 12  # physical resolution is tiny, so zoom in hard to stay readable
        self.logical_w, self.logical_h, self.canvas_w, self.canvas_h = next(iter(CANVAS_SIZES.values()))
        self.face_cls = next(iter(FACE_REGISTRY.values()))
        self.face = self.face_cls(self.logical_w, self.logical_h)
        self.buffer = [[self.face.sky_hex] * self.canvas_w for _ in range(self.canvas_h)]

        self._build_ui()
        self._bind_input()

        self.last_time = time.perf_counter()
        self.after(FRAME_MS, self._tick)

    def _build_ui(self) -> None:
        root = ttk.Frame(self, padding=10)
        root.grid(row=0, column=0, sticky="nsew")

        self.image = tk.PhotoImage(width=self.canvas_w, height=self.canvas_h)
        self.display_image = self.image.zoom(self.scale, self.scale)
        self.canvas_widget = tk.Canvas(
            root, width=self.canvas_w * self.scale, height=self.canvas_h * self.scale,
            highlightthickness=1, highlightbackground="#444",
        )
        self.canvas_widget.grid(row=0, column=0, rowspan=8, padx=(0, 12))
        self.canvas_image_id = self.canvas_widget.create_image(0, 0, anchor="nw", image=self.display_image)

        ttk.Label(root, text="Face").grid(row=0, column=1, sticky="w")
        self.face_var = tk.StringVar(value=self.face.name)
        face_combo = ttk.Combobox(root, textvariable=self.face_var, values=list(FACE_REGISTRY.keys()), state="readonly", width=26)
        face_combo.grid(row=1, column=1, sticky="we")
        face_combo.bind("<<ComboboxSelected>>", self._on_face_changed)

        ttk.Label(root, text="Canvas size").grid(row=2, column=1, sticky="w", pady=(10, 0))
        self.size_var = tk.StringVar(value=next(iter(CANVAS_SIZES.keys())))
        size_combo = ttk.Combobox(root, textvariable=self.size_var, values=list(CANVAS_SIZES.keys()), state="readonly", width=26)
        size_combo.grid(row=3, column=1, sticky="we")
        size_combo.bind("<<ComboboxSelected>>", self._on_size_changed)

        self.action_button = tk.Button(root, text=self.face.action_label, width=24, height=2)
        self.action_button.grid(row=4, column=1, pady=(16, 4))
        self.action_button.bind("<ButtonPress-1>", lambda _e: self.face.set_action_pressed(True))
        self.action_button.bind("<ButtonRelease-1>", lambda _e: self.face.set_action_pressed(False))

        reset_button = ttk.Button(root, text="Reset (R)", command=self._reset_face)
        reset_button.grid(row=5, column=1, sticky="we")

        ttk.Label(root, text="Hold SPACE or the button above\nto trigger the boop sensor.\nR resets the face.", justify="left").grid(row=6, column=1, sticky="w", pady=(12, 0))

        self.status_var = tk.StringVar(value="")
        ttk.Label(root, textvariable=self.status_var, font=("Consolas", 9)).grid(row=7, column=1, sticky="w", pady=(12, 0))

    def _bind_input(self) -> None:
        self.bind("<KeyPress-space>", lambda _e: self.face.set_action_pressed(True))
        self.bind("<KeyRelease-space>", lambda _e: self.face.set_action_pressed(False))
        self.bind("<KeyPress-r>", lambda _e: self._reset_face())
        self.bind("<KeyPress-R>", lambda _e: self._reset_face())

    def _reset_face(self) -> None:
        self.face.reset()

    def _on_face_changed(self, _event=None) -> None:
        self.face_cls = FACE_REGISTRY[self.face_var.get()]
        self.face = self.face_cls(self.logical_w, self.logical_h)
        self.action_button.config(text=self.face.action_label)

    def _on_size_changed(self, _event=None) -> None:
        self.logical_w, self.logical_h, self.canvas_w, self.canvas_h = CANVAS_SIZES[self.size_var.get()]
        self.image = tk.PhotoImage(width=self.canvas_w, height=self.canvas_h)
        self.canvas_widget.config(width=self.canvas_w * self.scale, height=self.canvas_h * self.scale)
        self.buffer = [[self.face.sky_hex] * self.canvas_w for _ in range(self.canvas_h)]
        self.face.set_size(self.logical_w, self.logical_h)

    def _tick(self) -> None:
        now = time.perf_counter()
        dt = now - self.last_time
        self.last_time = now

        self.face.update(dt)
        self.face.render(self.buffer, self.canvas_w, self.canvas_h)

        data = " ".join("{" + " ".join(row) + "}" for row in self.buffer)
        self.image.put(data)
        self.display_image = self.image.zoom(self.scale, self.scale)
        self.canvas_widget.itemconfig(self.canvas_image_id, image=self.display_image)

        self.status_var.set(self.face.status_text())

        self.after(FRAME_MS, self._tick)


if __name__ == "__main__":
    FaceSimulatorApp().mainloop()
