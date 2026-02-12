# USB Video Streaming (PC App)

This streams live frames from your PC to ProtoTracer over the USB cable.

## What this is (and is not)

- This is **USB serial video streaming** to a 64x32 feed.
- This is **not** native USB monitor mode (the Teensy does not enumerate as a standard HDMI/DisplayPort monitor).
- Streaming mode is entered automatically when the PC app starts and sends the start command.

## Firmware setup

1. In `lib/ProtoTracer/Examples/UserConfiguration.h`, keep:

```cpp
#define ENABLE_USB_VIDEO_FACE
```

2. Build/upload your HUB75 project firmware.
3. The app will automatically signal the Teensy to enter streaming mode.
4. When the app exits, it sends a stop command and the Teensy returns to normal face rendering.

## PC setup

From the repository root:

```bash
python -m pip install -r tools/usb_video_streamer_requirements.txt
```

Install the virtual monitor driver (Windows, one time):

```powershell
winget install --id VirtualDrivers.Virtual-Display-Driver --accept-source-agreements --accept-package-agreements
```

## Basic usage

List monitors:

```bash
python tools/usb_video_streamer.py --list-monitors
```

Stream monitor 2 at 30 FPS:

```bash
python tools/usb_video_streamer.py --port COM7 --source screen --monitor 2 --fps 30 --preview
```

Auto-pick the newest/last monitor (useful for virtual displays):

```bash
python tools/usb_video_streamer.py --source screen --monitor auto --fps 30 --preview
```

Stream a camera:

```bash
python tools/usb_video_streamer.py --port COM7 --source camera --camera-index 0 --fps 30 --preview
```

Stream a video file:

```bash
python tools/usb_video_streamer.py --port COM7 --source video --video-path demo.mp4 --video-loop --fps 30 --preview
```

## Useful options

- `--scale-mode fill|fit`:
  - `fill` crops to fully fill 64x32.
  - `fit` preserves aspect ratio with black bars.
- `--rotate 90|180|270` and `--flip-h` / `--flip-v` for orientation.
- `--single-panel` to disable lower-panel mirror (if firmware allows it).
- `--force-mirror` to force mirror to the lower panel.

## Performance notes

- Start with `--fps 30`.
- If frames tear or lag, reduce FPS (`--fps 20`).
- If the image looks stretched, switch between `--scale-mode fill` and `--scale-mode fit`.

## Quick launcher modes (Windows)

The batch launcher supports display mode switching before streaming:

```bat
tools\run_usb_stream.bat extend
tools\run_usb_stream.bat mirror
tools\run_usb_stream.bat second
```

- `extend` = extended desktop.
- `mirror` = cloned desktop.
- `second` = second-screen-only.
- If no mode is provided, it keeps your current Windows display mode.
- `extend` and `second` will prompt for Administrator access to ensure the virtual display driver is installed/enabled.
- Mode defaults: `mirror` captures monitor `1`; `extend`/`second` capture monitor `2` (override by passing your own args).

## Mirror-only executable (Windows)

If you only want desktop mirroring, build the dedicated mirror EXE:

```bat
tools\build_usb_mirror_exe.bat
```

Output:

```text
tools\dist\ProtoTracerUSBMirror.exe
```

Run it with no arguments for primary-monitor mirroring at 30 FPS:

```bat
tools\dist\ProtoTracerUSBMirror.exe
```

Optional examples:

```bat
tools\dist\ProtoTracerUSBMirror.exe --preview
tools\dist\ProtoTracerUSBMirror.exe --monitor 1 --fps 24
tools\dist\ProtoTracerUSBMirror.exe --list-monitors
```
