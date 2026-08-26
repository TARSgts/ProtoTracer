#!/usr/bin/env python3
"""ProtoTracer HUB75 Face Preview Tool.

Parses NukudeFace.h directly, applies morph weights like firmware,
and rasterizes to a 192x94 preview buffer.
"""

from __future__ import annotations

import math
import re
import json
import tkinter as tk
from dataclasses import dataclass
from pathlib import Path
from tkinter import filedialog, ttk

HUB75_W = 192
HUB75_H = 94
SCALE = 4
CANVAS_W = HUB75_W * SCALE
CANVAS_H = HUB75_H * SCALE

THIS_DIR = Path(__file__).resolve().parent
REPO_ROOT = THIS_DIR.parent.parent
NUKUDE_HEADER = REPO_ROOT / "lib" / "ProtoTracer" / "Assets" / "Models" / "FBX" / "NukudeFace.h"


@dataclass
class MorphData:
    name: str
    indexes: list[int]
    vectors: list[tuple[float, float, float]]


class MeshData:
    def __init__(self) -> None:
        self.vertices: list[tuple[float, float, float]] = []
        self.base_vertices: list[tuple[float, float, float]] = []
        self.triangles: list[tuple[int, int, int]] = []
        self.morphs: dict[str, MorphData] = {}


class Hub75FacePreview(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("ProtoTracer HUB75 Face Preview")
        self.geometry("1180x640")
        self.minsize(1120, 560)

        self.mesh = self._parse_nukude_header(NUKUDE_HEADER)
        self._init_projection()

        self.weights = {
            "Frown": tk.DoubleVar(value=0.0),
            "Doubt": tk.DoubleVar(value=0.0),
            "Surprised": tk.DoubleVar(value=0.0),
            "Sadness": tk.DoubleVar(value=0.0),
            "Anger": tk.DoubleVar(value=0.0),
            "LookDown": tk.DoubleVar(value=0.0),
            "LookUp": tk.DoubleVar(value=0.0),
            "Blink": tk.DoubleVar(value=0.0),
            "HideBlush": tk.DoubleVar(value=1.0),
            "vrc_v_aa": tk.DoubleVar(value=0.0),
            "vrc_v_ee": tk.DoubleVar(value=0.0),
            "vrc_v_oh": tk.DoubleVar(value=0.0),
        }
        self.visible_sliders = [
            "Frown",
            "Doubt",
            "Surprised",
            "Sadness",
            "Anger",
            "LookDown",
            "LookUp",
            "Blink",
            "vrc_v_aa",
            "vrc_v_ee",
            "vrc_v_oh",
        ]
        self.blush_visible = tk.DoubleVar(value=0.0)
        self.surprised_blush_toggle = tk.BooleanVar(value=False)

        self.animate_var = tk.BooleanVar(value=True)
        self._time = 0.0
        self.edit_mode = tk.BooleanVar(value=False)
        self.base_edit_mode = tk.BooleanVar(value=False)
        self.selected_edit_morph = tk.StringVar(value="Blink")
        self._active_drag_vertex: int | None = None
        self._last_proj: list[tuple[float, float, float]] = []
        self.morph_point_tool = tk.StringVar(value="move")
        self.selected_vertices: set[int] = set()
        self.drag_mode: str = "none"  # none|marquee|move_one|move_group
        self.marquee_start: tuple[int, int] | None = None
        self.marquee_current: tuple[int, int] | None = None
        self.group_last_mouse: tuple[int, int] | None = None

        self._build_ui()
        self._tick()

    def _parse_nukude_header(self, path: Path) -> MeshData:
        text = path.read_text(encoding="utf-8", errors="ignore")
        mesh = MeshData()

        bv = re.search(r"basisVertices\[\d+\]\s*=\s*\{(.*?)\};", text, re.S)
        if not bv:
            raise RuntimeError("basisVertices not found")
        vecs = re.findall(r"Vector3D\(([-\d\.]+)f,([-\d\.]+)f,([-\d\.]+)f\)", bv.group(1))
        mesh.vertices = [(float(x), float(y), float(z)) for x, y, z in vecs]
        mesh.base_vertices = list(mesh.vertices)

        bi = re.search(r"basisIndexes\[\d+\]\s*=\s*\{(.*?)\};", text, re.S)
        if not bi:
            raise RuntimeError("basisIndexes not found")
        tris = re.findall(r"IndexGroup\((\d+),(\d+),(\d+)\)", bi.group(1))
        mesh.triangles = [(int(a), int(b), int(c)) for a, b, c in tris]

        idx_matches = re.findall(r"int\s+([A-Za-z0-9_]+)Indexes\[\d+\]\s*=\s*\{([^}]*)\};", text, re.S)
        vec_matches = re.findall(r"Vector3D\s+([A-Za-z0-9_]+)Vectors\[\d+\]\s*=\s*\{(.*?)\};", text, re.S)

        idx_map: dict[str, list[int]] = {}
        for name, body in idx_matches:
            idx_map[name] = [int(v.strip()) for v in body.split(",") if v.strip()]

        vec_map: dict[str, list[tuple[float, float, float]]] = {}
        for name, body in vec_matches:
            m = re.findall(r"Vector3D\(([-\d\.]+)f,([-\d\.]+)f,([-\d\.]+)f\)", body)
            vec_map[name] = [(float(x), float(y), float(z)) for x, y, z in m]

        for name, indexes in idx_map.items():
            if name in vec_map and len(indexes) == len(vec_map[name]):
                mesh.morphs[name] = MorphData(name=name, indexes=indexes, vectors=vec_map[name])

        return mesh

    def _build_ui(self) -> None:
        self._apply_dark_theme()
        self.columnconfigure(0, weight=0)
        self.columnconfigure(1, weight=1)
        self.rowconfigure(0, weight=1)

        sidebar = ttk.Frame(self, padding=(10, 10, 6, 10))
        sidebar.grid(row=0, column=0, sticky="ns")
        sidebar.rowconfigure(0, weight=1)
        sidebar.columnconfigure(0, weight=1)

        side_canvas = tk.Canvas(sidebar, width=320, highlightthickness=0, bg="#1E1E1E")
        side_canvas.grid(row=0, column=0, sticky="ns")
        side_scroll = ttk.Scrollbar(sidebar, orient="vertical", command=side_canvas.yview)
        side_scroll.grid(row=0, column=1, sticky="ns")
        side_canvas.configure(yscrollcommand=side_scroll.set)

        panel = ttk.Frame(side_canvas, padding=4)
        panel_window = side_canvas.create_window((0, 0), window=panel, anchor="nw")

        def _sync_scroll_region(_event: tk.Event) -> None:
            side_canvas.configure(scrollregion=side_canvas.bbox("all"))

        def _fit_panel_width(event: tk.Event) -> None:
            side_canvas.itemconfigure(panel_window, width=event.width)

        panel.bind("<Configure>", _sync_scroll_region)
        side_canvas.bind("<Configure>", _fit_panel_width)

        ttk.Label(panel, text="NukudeFace Morphs", font=("Segoe UI", 11, "bold")).pack(anchor="w", pady=(0, 8))

        for key in self.visible_sliders:
            var = self.weights[key]
            row = ttk.Frame(panel)
            row.pack(fill="x", pady=2)
            ttk.Label(row, text=key, width=10).pack(side="left")
            s = ttk.Scale(row, from_=0.0, to=1.0, variable=var, command=lambda _v: self.render())
            s.pack(side="left", fill="x", expand=True)

        blush_row = ttk.Frame(panel)
        blush_row.pack(fill="x", pady=(8, 2))
        ttk.Label(blush_row, text="Blush", width=10).pack(side="left")
        ttk.Scale(
            blush_row,
            from_=0.0,
            to=1.0,
            variable=self.blush_visible,
            command=self._on_blush_visible_change,
        ).pack(side="left", fill="x", expand=True)

        ttk.Checkbutton(panel, text="Surprised / Blush", variable=self.surprised_blush_toggle, command=self._on_surprised_blush_toggle).pack(anchor="w", pady=(10, 2))
        ttk.Checkbutton(panel, text="Animate (default-like)", variable=self.animate_var).pack(anchor="w", pady=(2, 4))
        ttk.Separator(panel, orient="horizontal").pack(fill="x", pady=8)
        ttk.Label(panel, text="Morph Editor", font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(0, 2))
        self.edit_morphs_check = ttk.Checkbutton(
            panel,
            text="Edit Morphs",
            variable=self.edit_mode,
            command=self._on_toggle_edit_mode,
        )
        self.edit_morphs_check.pack(anchor="w", pady=2)

        self.morph_controls = ttk.Frame(panel)
        self.morph_controls.pack(fill="x", pady=(4, 2))
        morph_names = sorted(self.mesh.morphs.keys())
        self.morph_combo = ttk.Combobox(
            self.morph_controls,
            textvariable=self.selected_edit_morph,
            values=morph_names,
            state="readonly",
        )
        self.morph_combo.pack(fill="x", pady=2)
        self.morph_combo.bind("<<ComboboxSelected>>", self._on_morph_selected)
        ttk.Button(self.morph_controls, text="Load Morph For Editing", command=self._load_selected_morph_for_edit).pack(fill="x", pady=2)
        ttk.Button(self.morph_controls, text="Create Custom Shape Morph", command=self._create_custom_shape_morph).pack(fill="x", pady=2)
        ttk.Label(
            self.morph_controls,
            text="Point Tool",
            wraplength=280,
            foreground="#AAA",
        ).pack(anchor="w", pady=(2, 2))
        tool_row = ttk.Frame(self.morph_controls)
        tool_row.pack(fill="x", pady=(0, 4))
        ttk.Radiobutton(tool_row, text="Move", value="move", variable=self.morph_point_tool).pack(side="left")
        ttk.Radiobutton(tool_row, text="Add", value="add", variable=self.morph_point_tool).pack(side="left", padx=(8, 0))
        ttk.Radiobutton(tool_row, text="Remove", value="remove", variable=self.morph_point_tool).pack(side="left", padx=(8, 0))
        ttk.Label(
            self.morph_controls,
            text="Move is safe default. Add/Remove only act when explicitly selected.\nDrag empty space to box-select, then drag a selected point to move group.",
            wraplength=280,
            foreground="#888",
        ).pack(anchor="w", pady=(0, 2))
        ttk.Button(self.morph_controls, text="Save Morph Patch JSON", command=self._save_morph_patch).pack(fill="x", pady=2)
        self.morph_controls.pack_forget()

        self.base_edit_check = ttk.Checkbutton(
            panel,
            text="Base Vertex Edit Mode",
            variable=self.base_edit_mode,
            command=self._on_toggle_base_edit_mode,
        )
        self.base_edit_check.pack(anchor="w", pady=2)

        ttk.Button(panel, text="Save Base Vertices Patch JSON", command=self._save_base_patch).pack(fill="x", pady=2)

        ttk.Label(panel, text="Presets", font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(8, 2))
        ttk.Button(panel, text="Default", command=self._preset_default).pack(fill="x", pady=2)
        ttk.Button(panel, text="Reset", command=self._reset).pack(fill="x", pady=2)

        self.status = tk.StringVar(value=f"Loaded mesh: {len(self.mesh.vertices)} verts, {len(self.mesh.triangles)} tris")
        ttk.Label(panel, textvariable=self.status, wraplength=260, foreground="#444").pack(anchor="w", pady=(10, 0))

        self.canvas = tk.Canvas(self, width=CANVAS_W, height=CANVAS_H, bg="#101114", highlightthickness=0)
        self.canvas.grid(row=0, column=1, sticky="nsew", padx=(0, 10), pady=10)
        self.canvas.bind("<Button-1>", self._on_canvas_down)
        self.canvas.bind("<B1-Motion>", self._on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_canvas_up)
        self.canvas.bind("<Configure>", lambda _e: self.render())

    def _init_projection(self) -> None:
        xs = [v[0] for v in self.mesh.vertices]
        ys = [v[1] for v in self.mesh.vertices]
        zs = [v[2] for v in self.mesh.vertices]
        self.min_x, self.max_x = min(xs), max(xs)
        self.min_y, self.max_y = min(ys), max(ys)
        self.min_z, self.max_z = min(zs), max(zs)
        self.span_x = max(self.max_x - self.min_x, 1e-6)
        self.span_y = max(self.max_y - self.min_y, 1e-6)
        self.span_z = max(self.max_z - self.min_z, 1e-6)
        self.sx = (HUB75_W - 1) / self.span_x
        self.sy = (HUB75_H - 1) / self.span_y

    def _reset(self) -> None:
        for var in self.weights.values():
            var.set(0.0)
        self.blush_visible.set(0.0)
        self.weights["HideBlush"].set(1.0)
        self.surprised_blush_toggle.set(False)
        self.render()

    def _on_toggle_edit_mode(self) -> None:
        if self.edit_mode.get():
            if self.base_edit_mode.get():
                self.base_edit_mode.set(False)
            self._reset()
            self.animate_var.set(False)
            self._load_selected_morph_for_edit()
            self.morph_point_tool.set("move")
            self.selected_vertices.clear()
            self.morph_controls.pack(after=self.edit_morphs_check, fill="x", pady=(4, 2))
            self.status.set(f"Edit mode ON. Neutral pose loaded for morph: {self.selected_edit_morph.get()}")
        else:
            self.morph_controls.pack_forget()
            self.selected_vertices.clear()
            self.status.set("Edit mode OFF.")
        self.render()

    def _on_toggle_base_edit_mode(self) -> None:
        if self.base_edit_mode.get():
            if self.edit_mode.get():
                self.edit_mode.set(False)
            self.morph_controls.pack_forget()
            self._reset()
            self.animate_var.set(False)
            self.selected_vertices.clear()
            self.status.set("Base vertex edit mode ON. Drag handles to sculpt neutral face.")
        else:
            self.selected_vertices.clear()
            self.status.set("Base vertex edit mode OFF.")
        self.render()

    def _on_morph_selected(self, _event: tk.Event) -> None:
        if not self.edit_mode.get():
            return
        self._load_selected_morph_for_edit()

    def _on_surprised_blush_toggle(self) -> None:
        if self.surprised_blush_toggle.get():
            self._preset_surprised_blush()
        else:
            self._preset_default()

    def _load_selected_morph_for_edit(self) -> None:
        morph = self.selected_edit_morph.get()
        # Neutralize everything while editing so the selected feature is isolated.
        for var in self.weights.values():
            var.set(0.0)
        self.weights["HideBlush"].set(1.0)
        self.blush_visible.set(0.0)
        # Ensure any selected morph can be forced to target state even if it has no visible slider.
        if morph not in self.weights:
            self.weights[morph] = tk.DoubleVar(value=0.0)
        self.weights[morph].set(1.0)
        self.status.set(f"Editing morph: {morph}. Drag white handles to reshape.")
        self.render()

    def _create_custom_shape_morph(self) -> None:
        src = self.selected_edit_morph.get()
        src_data = self.mesh.morphs.get(src)
        if src_data is None:
            self.status.set("No source morph selected to clone.")
            return
        base = src.replace(" ", "")
        name = f"Custom{base}"
        suffix = 1
        while name in self.mesh.morphs:
            suffix += 1
            name = f"Custom{base}{suffix}"
        new_vectors = [(0.0, 0.0, 0.0) for _ in src_data.indexes]
        self.mesh.morphs[name] = MorphData(name=name, indexes=list(src_data.indexes), vectors=new_vectors)
        self.weights[name] = tk.DoubleVar(value=0.0)
        self.selected_edit_morph.set(name)
        self._load_selected_morph_for_edit()
        self.status.set(f"Created custom morph: {name}")

    def _save_morph_patch(self) -> None:
        morph = self.selected_edit_morph.get()
        data = self.mesh.morphs.get(morph)
        if data is None:
            return
        out = {
            "morph": morph,
            "indexes": data.indexes,
            "vectors": data.vectors,
        }
        path = filedialog.asksaveasfilename(
            title="Save morph patch",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        Path(path).write_text(json.dumps(out, indent=2), encoding="utf-8")
        self.status.set(f"Saved morph patch: {Path(path).name}")

    def _save_base_patch(self) -> None:
        changed = []
        for i, (b, v) in enumerate(zip(self.mesh.base_vertices, self.mesh.vertices)):
            dx = v[0] - b[0]
            dy = v[1] - b[1]
            dz = v[2] - b[2]
            if abs(dx) > 1e-6 or abs(dy) > 1e-6 or abs(dz) > 1e-6:
                changed.append({"index": i, "delta": [dx, dy, dz], "vertex": [v[0], v[1], v[2]]})

        out = {
            "type": "base_vertices_patch",
            "count_changed": len(changed),
            "changes": changed,
        }
        path = filedialog.asksaveasfilename(
            title="Save base vertices patch",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        Path(path).write_text(json.dumps(out, indent=2), encoding="utf-8")
        self.status.set(f"Saved base patch: {Path(path).name} ({len(changed)} verts changed)")

    def _on_blush_visible_change(self, _value: str) -> None:
        # Model uses HideBlush: 0=visible, 1=hidden. Expose a user-facing Blush slider.
        self.weights["HideBlush"].set(1.0 - float(self.blush_visible.get()))
        self.render()

    def _preset_default(self) -> None:
        # HUB75 Default() -> neutral face color/material with no expression morph weights.
        self._reset()

    def _preset_surprised_blush(self) -> None:
        # Mirrors ProtogenHUB75Project::Surprised():
        # Surprised = 1.0 and HideBlush = 0.0 (blush visible).
        if self.edit_mode.get() or self.base_edit_mode.get():
            return
        for var in self.weights.values():
            var.set(0.0)
        self.weights["Surprised"].set(1.0)
        self.weights["HideBlush"].set(0.0)
        self.blush_visible.set(1.0)
        self.surprised_blush_toggle.set(True)
        self.render()

    def _apply_morphs(self) -> list[tuple[float, float, float]]:
        verts = [list(v) for v in self.mesh.vertices]
        for name, var in self.weights.items():
            w = float(var.get())
            if w <= 0.0:
                continue
            m = self.mesh.morphs.get(name)
            if not m:
                continue
            for i, vidx in enumerate(m.indexes):
                dx, dy, dz = m.vectors[i]
                verts[vidx][0] += dx * w
                verts[vidx][1] += dy * w
                verts[vidx][2] += dz * w
        return [(x, y, z) for x, y, z in verts]

    def _project_verts(self, verts: list[tuple[float, float, float]]) -> list[tuple[float, float, float]]:
        proj: list[tuple[float, float, float]] = []
        for x, y, z in verts:
            px = (x - self.min_x) * self.sx
            py = (y - self.min_y) * self.sy
            py = (HUB75_H - 1) - py
            pz = (z - self.min_z) / self.span_z
            proj.append((px, py, pz))
        return proj

    @staticmethod
    def _point_in_triangle(px: float, py: float, a: tuple[float, float], b: tuple[float, float], c: tuple[float, float]) -> bool:
        ax, ay = a
        bx, by = b
        cx, cy = c
        v0x, v0y = cx - ax, cy - ay
        v1x, v1y = bx - ax, by - ay
        v2x, v2y = px - ax, py - ay
        dot00 = v0x * v0x + v0y * v0y
        dot01 = v0x * v1x + v0y * v1y
        dot02 = v0x * v2x + v0y * v2y
        dot11 = v1x * v1x + v1y * v1y
        dot12 = v1x * v2x + v1y * v2y
        denom = dot00 * dot11 - dot01 * dot01
        if abs(denom) < 1e-9:
            return False
        inv = 1.0 / denom
        u = (dot11 * dot02 - dot01 * dot12) * inv
        v = (dot00 * dot12 - dot01 * dot02) * inv
        return u >= 0 and v >= 0 and (u + v) <= 1

    def _rasterize(self, verts: list[tuple[float, float, float]]) -> list[list[int]]:
        proj = self._project_verts(verts)
        self._last_proj = proj

        zbuf = [[-1e9 for _ in range(HUB75_W)] for _ in range(HUB75_H)]
        img = [[0 for _ in range(HUB75_W)] for _ in range(HUB75_H)]

        for ia, ib, ic in self.mesh.triangles:
            ax, ay, az = proj[ia]
            bx, by, bz = proj[ib]
            cx, cy, cz = proj[ic]

            min_px = max(0, int(min(ax, bx, cx)))
            max_px = min(HUB75_W - 1, int(max(ax, bx, cx)) + 1)
            min_py = max(0, int(min(ay, by, cy)))
            max_py = min(HUB75_H - 1, int(max(ay, by, cy)) + 1)

            brightness = int((az + bz + cz) / 3.0 * 180 + 60)

            for yy in range(min_py, max_py + 1):
                for xx in range(min_px, max_px + 1):
                    if not self._point_in_triangle(xx + 0.5, yy + 0.5, (ax, ay), (bx, by), (cx, cy)):
                        continue
                    z = (az + bz + cz) / 3.0
                    if z > zbuf[yy][xx]:
                        zbuf[yy][xx] = z
                        img[yy][xx] = brightness

        return img

    def render(self) -> None:
        verts = self._apply_morphs()
        img = self._rasterize(verts)

        self.canvas.delete("all")
        cw = max(1, int(self.canvas.winfo_width()))
        ch = max(1, int(self.canvas.winfo_height()))
        scale = max(1, min(cw // HUB75_W, ch // HUB75_H))
        draw_w = HUB75_W * scale
        draw_h = HUB75_H * scale
        ox = (cw - draw_w) // 2
        oy = (ch - draw_h) // 2

        # Visible working-area border for the HUB75 frame limits.
        self.canvas.create_rectangle(
            ox - 1,
            oy - 1,
            ox + draw_w + 1,
            oy + draw_h + 1,
            outline="#4FC3F7",
            width=2,
        )

        for y in range(HUB75_H):
            yy = oy + y * scale
            for x in range(HUB75_W):
                v = img[y][x]
                if v <= 0:
                    continue
                c = f"#{v:02x}{v:02x}{v:02x}"
                xx = ox + x * scale
                self.canvas.create_rectangle(xx, yy, xx + scale, yy + scale, outline="", fill=c)

        if self.edit_mode.get() or self.base_edit_mode.get():
            self._draw_edit_handles(scale, ox, oy)

    def _draw_edit_handles(self, scale: int, ox: int, oy: int) -> None:
        if self.base_edit_mode.get():
            for vidx, (px, py, _pz) in enumerate(self._last_proj):
                x = int(ox + px * scale)
                y = int(oy + py * scale)
                r = 2
                if vidx in self.selected_vertices:
                    self.canvas.create_oval(x - r - 1, y - r - 1, x + r + 1, y + r + 1, outline="#FFD54F", fill="#FFD54F")
                else:
                    self.canvas.create_oval(x - r, y - r, x + r, y + r, outline="#8FD3FF", fill="#8FD3FF")
            return

        morph = self.selected_edit_morph.get()
        data = self.mesh.morphs.get(morph)
        if data is None or not self._last_proj:
            return
        for vidx in data.indexes:
            px, py, _pz = self._last_proj[vidx]
            x = int(ox + px * scale)
            y = int(oy + py * scale)
            r = 4
            if vidx in self.selected_vertices:
                self.canvas.create_oval(x - r - 1, y - r - 1, x + r + 1, y + r + 1, outline="#FFD54F", fill="#FFD54F")
            else:
                self.canvas.create_oval(x - r, y - r, x + r, y + r, outline="#FFFFFF", fill="#FFFFFF")

        if self.drag_mode == "marquee" and self.marquee_start and self.marquee_current:
            x0, y0 = self.marquee_start
            x1, y1 = self.marquee_current
            self.canvas.create_rectangle(x0, y0, x1, y1, outline="#90CAF9", width=1, dash=(3, 3))

    def _on_canvas_down(self, event: tk.Event) -> None:
        if not self.edit_mode.get() and not self.base_edit_mode.get():
            return
        if not self._last_proj:
            return
        force_marquee = (event.state & 0x0001) != 0  # Shift key
        cw = max(1, int(self.canvas.winfo_width()))
        ch = max(1, int(self.canvas.winfo_height()))
        scale = max(1, min(cw // HUB75_W, ch // HUB75_H))
        ox = (cw - HUB75_W * scale) // 2
        oy = (ch - HUB75_H * scale) // 2
        if self.base_edit_mode.get():
            candidate_indexes = list(range(len(self._last_proj)))
        else:
            morph = self.selected_edit_morph.get()
            data = self.mesh.morphs.get(morph)
            if data is None:
                return
            candidate_indexes = data.indexes

        best_v = None
        best_d2 = 12 * 12
        for vidx in candidate_indexes:
            px, py, _ = self._last_proj[vidx]
            sx = ox + px * scale
            sy = oy + py * scale
            d2 = (sx - event.x) ** 2 + (sy - event.y) ** 2
            if d2 < best_d2:
                best_d2 = d2
                best_v = vidx

        if force_marquee:
            best_v = None

        if not self.base_edit_mode.get() and best_v is not None:
            tool = self.morph_point_tool.get()
            if tool == "add":
                self._add_vertex_to_current_morph(best_v)
                return
            if tool == "remove":
                self._remove_vertex_from_current_morph(best_v)
                return

        if best_v is None:
            self.drag_mode = "marquee"
            self.marquee_start = (event.x, event.y)
            self.marquee_current = (event.x, event.y)
            self.selected_vertices.clear()
            self._active_drag_vertex = None
            self.render()
            return

        if best_v in self.selected_vertices and len(self.selected_vertices) > 1:
            self.drag_mode = "move_group"
            self.group_last_mouse = (event.x, event.y)
            self._active_drag_vertex = best_v
            return

        self.selected_vertices = {best_v}
        self.drag_mode = "move_one"
        self._active_drag_vertex = best_v
        self.render()

    def _add_vertex_to_current_morph(self, vidx: int) -> None:
        morph = self.selected_edit_morph.get()
        data = self.mesh.morphs.get(morph)
        if data is None:
            return
        if vidx in data.indexes:
            self.status.set(f"Vertex {vidx} is already in morph {morph}.")
            return
        data.indexes.append(vidx)
        data.vectors.append((0.0, 0.0, 0.0))
        self.status.set(f"Added vertex {vidx} to morph {morph}.")
        self.render()

    def _remove_vertex_from_current_morph(self, vidx: int) -> None:
        morph = self.selected_edit_morph.get()
        data = self.mesh.morphs.get(morph)
        if data is None:
            return
        if vidx not in data.indexes:
            self.status.set(f"Vertex {vidx} is not in morph {morph}.")
            return
        i = data.indexes.index(vidx)
        data.indexes.pop(i)
        data.vectors.pop(i)
        self.status.set(f"Removed vertex {vidx} from morph {morph}.")
        self.render()

    def _on_canvas_drag(self, event: tk.Event) -> None:
        if (not self.edit_mode.get() and not self.base_edit_mode.get()) or self._active_drag_vertex is None:
            if self.drag_mode == "marquee" and self.marquee_start:
                self.marquee_current = (event.x, event.y)
                self.render()
            return

        cw = max(1, int(self.canvas.winfo_width()))
        ch = max(1, int(self.canvas.winfo_height()))
        scale = max(1, min(cw // HUB75_W, ch // HUB75_H))
        ox = (cw - HUB75_W * scale) // 2
        oy = (ch - HUB75_H * scale) // 2
        # Compare against projected current vertex, then convert screen delta to model-space delta.
        px, py, _pz = self._last_proj[self._active_drag_vertex]
        current_sx = ox + px * scale
        current_sy = oy + py * scale
        dsx = event.x - current_sx
        dsy = event.y - current_sy
        dx_model = dsx / max(scale * self.sx, 1e-6)
        dy_model = -dsy / max(scale * self.sy, 1e-6)

        if self.drag_mode == "move_group" and self.group_last_mouse is not None:
            last_x, last_y = self.group_last_mouse
            dsx = event.x - last_x
            dsy = event.y - last_y
            dx_model = dsx / max(scale * self.sx, 1e-6)
            dy_model = -dsy / max(scale * self.sy, 1e-6)
            self.group_last_mouse = (event.x, event.y)

            if self.base_edit_mode.get():
                for vidx in self.selected_vertices:
                    vx, vy, vz = self.mesh.vertices[vidx]
                    self.mesh.vertices[vidx] = (vx + dx_model, vy + dy_model, vz)
            else:
                morph = self.selected_edit_morph.get()
                data = self.mesh.morphs.get(morph)
                if data is None:
                    return
                idx_map = {v: i for i, v in enumerate(data.indexes)}
                for vidx in self.selected_vertices:
                    if vidx not in idx_map:
                        continue
                    i = idx_map[vidx]
                    vx, vy, vz = data.vectors[i]
                    data.vectors[i] = (vx + dx_model, vy + dy_model, vz)
            self.render()
            return

        if self.base_edit_mode.get():
            vx, vy, vz = self.mesh.vertices[self._active_drag_vertex]
            self.mesh.vertices[self._active_drag_vertex] = (vx + dx_model, vy + dy_model, vz)
        else:
            morph = self.selected_edit_morph.get()
            data = self.mesh.morphs.get(morph)
            if data is None:
                return
            try:
                entry_i = data.indexes.index(self._active_drag_vertex)
            except ValueError:
                return
            vx, vy, vz = data.vectors[entry_i]
            data.vectors[entry_i] = (vx + dx_model, vy + dy_model, vz)
        self.render()

    def _on_canvas_up(self, _event: tk.Event) -> None:
        if self.drag_mode == "marquee" and self.marquee_start and self.marquee_current:
            self._select_vertices_in_marquee()
        self.drag_mode = "none"
        self.marquee_start = None
        self.marquee_current = None
        self.group_last_mouse = None
        self._active_drag_vertex = None
        self.render()

    def _select_vertices_in_marquee(self) -> None:
        if not self.marquee_start or not self.marquee_current:
            return
        x0, y0 = self.marquee_start
        x1, y1 = self.marquee_current
        left, right = min(x0, x1), max(x0, x1)
        top, bottom = min(y0, y1), max(y0, y1)

        cw = max(1, int(self.canvas.winfo_width()))
        ch = max(1, int(self.canvas.winfo_height()))
        scale = max(1, min(cw // HUB75_W, ch // HUB75_H))
        ox = (cw - HUB75_W * scale) // 2
        oy = (ch - HUB75_H * scale) // 2

        if self.base_edit_mode.get():
            candidate_indexes = list(range(len(self._last_proj)))
        else:
            morph = self.selected_edit_morph.get()
            data = self.mesh.morphs.get(morph)
            if data is None:
                return
            candidate_indexes = data.indexes

        selected: set[int] = set()
        for vidx in candidate_indexes:
            px, py, _ = self._last_proj[vidx]
            sx = ox + px * scale
            sy = oy + py * scale
            if left <= sx <= right and top <= sy <= bottom:
                selected.add(vidx)
        self.selected_vertices = selected

    def _tick(self) -> None:
        if self.edit_mode.get() and self.base_edit_mode.get():
            # Hard guard: never allow both edit modes at the same time.
            self.base_edit_mode.set(False)
        if self.edit_mode.get() or self.base_edit_mode.get():
            self.animate_var.set(False)
        if self.animate_var.get() and not self.edit_mode.get() and not self.base_edit_mode.get():
            self._time += 0.016
            blink = max(0.0, math.sin(self._time * 2.2)) ** 14
            self.weights["Blink"].set(blink)
            self.weights["vrc_v_aa"].set((math.sin(self._time * 6.0) + 1.0) * 0.25)
            self.weights["LookUp"].set((math.sin(self._time * 0.8) + 1.0) * 0.12)
            self.weights["LookDown"].set((math.sin(self._time * 0.8 + math.pi) + 1.0) * 0.08)
        self.render()
        self.after(16, self._tick)

    def _apply_dark_theme(self) -> None:
        self.configure(bg="#15171A")
        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass

        style.configure(".", background="#1E1E1E", foreground="#E6E6E6", fieldbackground="#2A2A2A")
        style.configure("TFrame", background="#1E1E1E")
        style.configure("TLabel", background="#1E1E1E", foreground="#E6E6E6")
        style.configure("TCheckbutton", background="#1E1E1E", foreground="#E6E6E6")
        style.configure("TRadiobutton", background="#1E1E1E", foreground="#E6E6E6")
        style.configure("TButton", background="#2C2F33", foreground="#F0F0F0")
        style.map("TButton", background=[("active", "#3A3F45")])
        style.configure("TCombobox", fieldbackground="#2A2A2A", background="#2A2A2A", foreground="#E6E6E6")
        style.configure("Horizontal.TScale", background="#1E1E1E")
        style.configure("Vertical.TScrollbar", background="#2C2F33", troughcolor="#1A1C1F")


def main() -> None:
    app = Hub75FacePreview()
    app.mainloop()


if __name__ == "__main__":
    main()
