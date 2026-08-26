# ProtoTracer Face Simulator

Previews the pixel logic of "auto-play" `Material` faces (the ones wired up via
`ENABLE_*_FACE` in `UserConfiguration.h` and `ProtogenProjectTemplate`) on the desktop,
without flashing hardware.

Each face is reimplemented in pure Python inside `face_simulator_app.py`, matching the
real `Update()`/`GetRGB()` C++ logic constant-for-constant. It is a separate
implementation from the firmware (not a compiled build of the real `.cpp`), so if you
tune constants in the C++ material, mirror the change here to keep the preview accurate.

## Run

```powershell
python tools/face_simulator/face_simulator_app.py
```

## Controls

- Hold `SPACE` (or click and hold the on-screen button) to trigger the boop sensor /
  action input — e.g. jump in the Dino Game face.
- `R` resets the current face.
- The dropdowns switch between registered faces and between the HUB75/WS35 canvas sizes.

## Adding another face to the simulator

Implement a class with this shape and add it to `FACE_REGISTRY`:

- `__init__(self, width, height)` / `set_size(self, width, height)` — mirrors the
  material's `SetSize()` (dimensions are the full canvas, not half-extents).
- `reset(self)` — mirrors the material's `Reset()`.
- `set_action_pressed(self, pressed: bool)` — mirrors the boop-sensor setter
  (`SetJumpPressed`, `SetLeverPulled`, etc.) if the face takes input; omit the call site
  in the app if the face is purely ambient.
- `update(self, dt: float)` — mirrors `Update()`.
- `render(self, buffer, canvas_w, canvas_h)` — paints into `buffer`, a list of rows of
  `"#rrggbb"` strings. Use the module-level `fill_rect()` helper to fill a rectangle
  given in the material's centered, Y-up coordinate space (matching `GetRGB`'s
  `relative` vector) — it handles the conversion to pixel rows/columns.
- `status_text(self) -> str` — one line shown under the canvas.

This mirrors the wiring pattern used for firmware faces themselves (see the project's
`adding-a-face-feature` notes): port the same constants from the `.cpp` file so behavior
matches what will actually run on the mask.
