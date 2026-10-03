# Embedded startup animation

HUB75 builds show the requested [Steam Deck boot animation](https://youtu.be/ms7Eq_bh2Tc)
once after initialization, then return to the current face. It is visual only;
the project has no audio playback output. Both 64x32 panels use the existing
panel orientation and brightness controls. Input, audio analysis and normal face
updates continue while the clip plays. Comment out `ENABLE_STARTUP_ANIMATION`
in `lib/ProtoTracer/Examples/UserConfiguration.h` to boot directly into faces.

The asset is Valve's original `steamui/movies/deck_startup.webm` from the installed
Steam client, visually matching the supplied video. It belongs to Valve.
`startup-asset.json` records the source hash and conversion details. The source
video and its soundtrack are not included in this repository.

The generated clip holds 96 frames at 24 FPS for four seconds. Each frame is
enlarged to use nearly the full panel height, with the original logo proportions
preserved. A fixed source crop removes most of the original black border;
the completed Steam logo remains fully visible with a small edge margin.
Larger animated outer rings can extend beyond the panel edges.
A shared 256-color palette, bounded run-length encoding and flash storage hold the asset
and 2,048 bytes for its decoded frame. Frames are selected by elapsed time;
late updates skip frames rather than blocking other work. A malformed frame
fails back to the normal face. Completion cannot restart on a menu change.

To regenerate, install Pillow and use a trusted ffmpeg executable:

```text
python tools/startup/convert_startup.py <deck_startup.webm> lib/ProtoTracer/Assets/Textures/Startup/SteamDeckStartup.h --ffmpeg <ffmpeg.exe> --preview-dir <output-folder> --crop 320 160 65 145 --fill-panel
```

The converter also writes a pixel-grid GIF, contact sheet and manifest for review.
The GIF loops for preview convenience; the firmware plays once. Its duration is
four seconds by design. `test_startup.cpp` runs the production decoder and player
with a minimal Arduino header, checking all frames, malformed input, timing,
rollover and one-shot completion. See `test_startup.py` for the host/WASM runner.
