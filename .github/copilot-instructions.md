# ProtoTracer AI Coding Instructions

## Architecture Overview
ProtoTracer is a real-time 3D rendering engine for Teensy microcontrollers driving LED displays (HUB75 panels or WS35 boards). Core components:
- **Engine/RenderingEngine**: Rasterizes scenes using CameraManager and Rasterizer
- **Scene**: Container for Object3D instances and optional screen-space Effects
- **Controller**: Hardware abstraction for display output (HUB75Controller, WS35Controller)
- **Animation**: KeyFrame-based animation system with EasyEaseAnimator and KeyFrameTrack
- **Camera**: Manages viewports and projections for multi-panel setups

Projects inherit from ProtogenProjectTemplate, configuring scenes, materials, and hardware in `Examples/Protogen/`.

## Build & Development Workflow
- Use PlatformIO: `platformio run --environment teensy41hub75` to build, `platformio run --target upload --environment teensy41hub75` to flash
- Environments: `teensy40hub75`, `teensy41ws35`, etc. - select based on board and display type
- Serial monitor: `platformio device monitor` for debugging output
- Tests: Unity-based in `test/`, run via PlatformIO test framework

## Key Conventions
- **Memory Management**: Use `DMAMEM` for large objects to place in Teensy's RAM2
- **Conditional Compilation**: Define projects via build flags (e.g., `PROJECT_PROTOGEN_HUB75`) in `platformio.ini`
- **Materials**: Static (SimpleMaterial) vs Animated (SpectrumAnalyzer, RainbowNoise) - animated materials react to audio/sensors
- **Assets**: Models in `Assets/Models/OBJ/`, textures in `Assets/Textures/`, converted via ProtoTracer-Helpers
- **Configuration**: Feature toggles in `Examples/UserConfiguration.h` (e.g., `#define PRINTINFO` for FPS stats)
- **Documentation**: Doxygen-style comments with `@file`, `@brief`, `@class` for all public APIs

## Common Patterns
- **Scene Setup**: Create Scene with max objects, add Object3D instances with materials and transforms
- **Animation**: Use KeyFrameTrack for property animation, attach to MaterialAnimator for material properties
- **Hardware Integration**: Controllers handle brightness ramping and display updates; cameras map to physical panels
- **Input Handling**: Sensors (MicrophoneFourier_MAX9814, APDS9960) feed into animated materials for reactivity

## Examples
- Add animated material: `scene.AddObject(new Object3D(model, new SpectrumAnalyzer(microphone)));`
- Keyframe animation: `KeyFrameTrack track; track.AddKeyFrame(0.0f, 0.0f); track.AddKeyFrame(1.0f, 1.0f);`
- Camera setup: Use HUB75DeltaCameras for dual-panel delta configuration

Focus on Teensy 4.x constraints: limited RAM/CPU, optimize for real-time rendering.</content>
<parameter name="filePath">c:\Users\carso\OneDrive\Documents\GitHub\ProtoTracer\.github\copilot-instructions.md