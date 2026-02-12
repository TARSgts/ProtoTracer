# ProtoTracer: 3D Rendering and Animation Engine

ProtoTracer is a real-time 3D rendering and animation engine designed for microcontrollers. While it has broad applicability, this is a project made for free and it will be down to you to customize support for other hardware other than the examples provided. Support for other microcontrollers may be possible through custom implementations, but users should be prepared to develop their own adaptations if working with platforms other than the Teensy 4/4.1.

[![Build and Release Firmware HEX Files](https://github.com/coelacant1/ProtoTracer/actions/workflows/build_firmware_and_release.yml/badge.svg)](https://github.com/coelacant1/ProtoTracer/actions/workflows/build_firmware_and_release.yml)
[![Generate Documentation](https://github.com/coelacant1/ProtoTracer/actions/workflows/documentation.yml/badge.svg)](https://github.com/coelacant1/ProtoTracer/actions/workflows/documentation.yml)
[![pages-build-deployment](https://github.com/coelacant1/ProtoTracer/actions/workflows/pages/pages-build-deployment/badge.svg)](https://github.com/coelacant1/ProtoTracer/actions/workflows/pages/pages-build-deployment)

## Quick Links

- [Usage](#usage)
- [USB Streaming Updates (Windows)](#usb-streaming-updates-windows)
- [Current change summary (from git working tree)](#current-change-summary-from-git-working-tree)
- [Questions and Support](#questions-and-support)

ProtoTracer supports:
- **64x32 HUB75 panels**
- **Custom panel designs based on WS2812B LEDs - WS35 Boards**

## Important Note
This project is complex and requires prior experience with microcontroller projects. If you're new to microcontrollers or seeking an easier solution, consider alternatives like:
- **Huidu WF-1**
  - 🗸 RGB, works with HUB75 panels
  - 🗸 Easy Wi-Fi configuration, quick setup with an app
  - ❌ Limited to static images, GIFs, and slideshows
  - ❌ No interactivity with sensors or buttons
- **MAX7219-based Protogen Designs**
  - 🗸 Affordable and widely available
  - 🗸 Compatible with Arduino Nano or similar controllers
  - 🗸 Interactive depending on firmware
  - 🗸 Low power consumption compared to HUB75 designs
  - ❌ Single-color, on/off pixels only
  - ❌ Requires significant soldering


# Frequently Asked Questions

## Can this project work on other microcontrollers?
- **Yes, but only the Teensy 4.0 and 4.1 are officially supported.** Users can develop custom implementations for other microcontrollers, but this requires in-depth technical expertise.

## Will this project work on a Raspberry Pi?
- **No**, ProtoTracer is not designed to run on a Raspberry Pi, but could be ported.

## Will this project work on an ESP32?
- **No**, ProtoTracer is not compatible with the ESP32, but could be ported.

## Will this project work on an Arduino Nano/Uno/Mega?
- **No**, ProtoTracer cannot practically run on these platforms due to hardware limitations.


# Demonstration

Here’s an example showcasing ProtoTracer's capabilities, demonstrating live rendering of a textured and rotating `.OBJ` file:

![SpyroExample](https://user-images.githubusercontent.com/77935580/130149757-41306da9-5296-42f5-86bc-87f785d9e56b.gif)


# Recommended Platform Requirements

- **Microcontroller:** Teensy 4.0 or Teensy 4.1
- **Panels:** Two 64x32 HUB75 panels or WS35 LED boards
- **Shield:** SmartMatrix V4 LED Shield, OctoWS2811 board, or ProtoController V2 (coming soon)

ProtoTracer has been tested on a Teensy 4.0 using a 2,000-triangle scene with a 4,096-pixel matrix.


# Usage

To get started with ProtoTracer, refer to the [ProtoTracer Documentation](https://coelacant1.github.io/ProtoTracer). It provides detailed instructions on:
- Importing files
- Setting up controllers
- Manipulating objects
- Rendering to displays

USB serial streaming to HUB75 (auto-switch when the PC app starts) is documented in:
- [`tutorial/USBVideoStreaming.md`](tutorial/USBVideoStreaming.md)

## USB Streaming Updates (Windows)

Recent USB streaming changes are now built into the repo and can be run directly from `tools/`.

### Firmware requirement

Enable USB video streaming in:

- `lib/ProtoTracer/Examples/UserConfiguration.h`

```cpp
#define ENABLE_USB_VIDEO_FACE
```

### Core streamer

- `tools/usb_video_streamer.py`

Features:
- Streams `screen`, `camera`, or `video` sources over USB serial.
- Sends start/stop control packets so the Teensy enters/leaves stream mode automatically.
- Supports monitor selector values for `--monitor`:
  - `primary`
  - `auto` (uses the newest/last monitor)
  - explicit monitor index (`1`, `2`, etc)

### Windows launcher modes

- `tools/run_usb_stream.bat`

Supported launch modes:
- `mirror`/`clone`: sets Windows clone mode and captures monitor `1` by default.
- `extend`: ensures the virtual display driver is ready, switches to extend mode, captures monitor `2`.
- `second`/`external`: ensures virtual display driver is ready, switches to second-screen-only, captures monitor `2`.
- `primary`: returns Windows to primary-only mode.

Behavior:
- Validates monitor availability for `extend` and `second`.
- Restores primary-only mode after `extend` or `second` stream sessions end.

### Virtual display helper

- `tools/ensure_virtual_display.ps1`

What it does:
- Installs/enables Virtual Display Driver (VDD) if needed.
- Detects `Root\\MttVDD` display devices.
- Removes duplicate VDD instances so repeated runs do not create many virtual monitors.

### Mirror-only executable flow

- `tools/usb_mirror_app.py`: mirror-focused entrypoint.
- `tools/build_usb_mirror_exe.bat`: builds a standalone EXE with PyInstaller.

Build:

```bat
tools\build_usb_mirror_exe.bat
```

Output:

```text
tools\dist\ProtoTracerUSBMirror.exe
```

Run (default mirror behavior):

```bat
tools\dist\ProtoTracerUSBMirror.exe
```

Optional:

```bat
tools\dist\ProtoTracerUSBMirror.exe --preview
tools\dist\ProtoTracerUSBMirror.exe --list-monitors
```

### Current change summary (from git working tree)

Modified files:
- `README.md`: added USB streaming update documentation and usage.
- `tools/run_usb_stream.bat`: added display mode handling (`mirror`, `extend`, `second`, `primary`), virtual-display readiness checks, monitor-count validation, and primary-display restore after streaming.
- `tools/usb_video_streamer.py`: added flexible monitor selector parsing (`auto`, `primary`, index) through `resolve_monitor_index`.
- `tutorial/USBVideoStreaming.md`: expanded setup/usage docs for launcher modes and mirror-only EXE workflow.

New files:
- `tools/ensure_virtual_display.ps1`: installs/enables VDD and removes duplicate `Root\\MttVDD` instances to avoid multiple phantom monitors.
- `tools/usb_mirror_app.py`: mirror-first entrypoint (defaults to primary monitor, 30 FPS).
- `tools/build_usb_mirror_exe.bat`: builds `ProtoTracerUSBMirror.exe` with PyInstaller.
- `tools/dist/ProtoTracerUSBMirror.exe`: built mirror app binary output.

Local untracked content not required for USB streaming:
- `tools/MCUME/` (separate local content).


# Customization

While ProtoTracer is designed for the Teensy 4 series, advanced users can create custom controllers for other microcontrollers. This requires a deep understanding of the codebase and the target microcontroller's capabilities. Refer to the [documentation](https://coelacant1.github.io/ProtoTracer) for guidance.

For the ProtoTracer Helper programs for converting FBX, OBJ, GIF, PNG/JPG, CSV to ProtoTracer compatible files, go to the [ProtoTracer Helper Github Repository](https://github.com/coelacant1/ProtoTracer-Helpers/tree/main)


# Questions and Support

For additional information or recommendations, use the **Discussions** tab on GitHub. For specific questions, join the project's [Discord server](https://discord.gg/YwaWnhJ). Please note:
- There is no dedicated support, this project is provided for free.
- Assistance is not guaranteed for problems specific to your custom setup, assume you are on your own.


# Contributing

Contributions are welcome! To contribute:
1. Fork the repository on GitHub.
2. Commit your changes with a descriptive message (git commit -m 'Add YourFeature').
3. Push the branch (git push origin main).
4. Submit a pull request on GitHub.


# Sponsoring
If you would like to help me keep this project going, any support on Patreon would be greatly appreciated: https://www.patreon.com/coelacant1


# License Agreement

ProtoTracer is licensed under the [AGPL-3.0](https://choosealicense.com/licenses/agpl-3.0/). This ensures modifications and contributions benefit the community. If you use or modify this software for a product, you must make the modified code publicly available as per the license.

