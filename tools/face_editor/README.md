# ProtoTracer HUB75 Face Preview + Morph Editor

This tool previews and edits the actual `NukudeFace` mesh/morphs used by HUB75.

## Run

```powershell
python tools/face_editor/face_editor_app.py
```

## Edit mode behavior

- Entering `Edit Mode` now forces a neutral/default pose.
- Animation is disabled while editing.
- All sliders are reset left (0) except internal `HideBlush=1` (hidden blush).

## Intuitive feature workflow

1. Pick a `Feature` (`Eyes`, `Mouth`, `Nose`, `Eyebrows`)
2. Click `Load Feature Target`
3. Turn on `Edit Mode (drag points)`
4. Drag white handles to reshape that feature morph
5. Save with `Save Morph Patch JSON`

## Custom shape support

- Click `Create Custom Shape Morph` to create a new editable morph for the selected feature region.
- Then sculpt it with drag handles and export as JSON patch.

## Notes

- `HideBlush` slider was removed from UI as requested; blush is controlled internally/default-hidden in edit mode.
- Existing presets still work (`Default`, `Surprised / Blush`).
