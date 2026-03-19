# XREAL AR Multiscreen

Native C++ application that streams multiple desktop windows into XREAL AR glasses as interactive virtual screens in 3D, with real-time 3DoF head tracking.

Rebuilt from an Electron/TypeScript prototype for lower latency, direct USB HID access, and tighter GPU control.

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Graphics | OpenGL 4.6 Core (GLFW + GLAD + GLM) |
| Window Capture | GDI PrintWindow (30 FPS), with planned WinRT upgrade |
| USB HID | hidapi (winapi backend) |
| Build | CMake 3.24+ with FetchContent |
| Language | C++20, MSVC |
| Platform | Windows 10/11 |

## Prerequisites

- **CMake 3.24+**
- **Visual Studio 2022 Build Tools** (MSVC C++ toolchain)
- **GPU with OpenGL 4.6 support** (any modern NVIDIA/AMD/Intel)
- **XREAL AR glasses** (Air, Air 2, Air 2 Pro, or Ultra) — required for head tracking

### Install Build Tools (Chocolatey)

Open **PowerShell as Administrator**:

```powershell
choco install cmake --installargs "ADD_CMAKE_TO_PATH=System" -y
choco install visualstudio2022buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" -y
```

Restart your terminal after installing so PATH updates take effect.

### Install Build Tools (winget)

```powershell
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive"
```

## Build & Run

```bash
# Configure (downloads GLFW, GLM, hidapi automatically via FetchContent)
cmake -B build -G "Visual Studio 17 2022"

# Build
cmake --build build --config Release

# Run
./build/Release/xr_ar_space.exe
```

Or from the Visual Studio Developer Command Prompt:

```cmd
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
build\Release\xr_ar_space.exe
```

## Controls

### Keyboard

| Key | Action |
|-----|--------|
| Arrow Left/Right | Rotate camera yaw |
| Arrow Up/Down | Rotate camera pitch |
| Tab | Cycle selected screen |
| 1-4 | Switch layout (Arc/Grid/Stack/Single) |
| R | Reset camera orientation |
| Escape | Exit |

### Mouse

| Input | Action |
|-------|--------|
| Hover | Highlight screen under cursor |
| Click + Drag | Move selected screen |
| Scroll Wheel | Zoom in/out |
| Double-click | Zoom to screen |

## Project Structure

```
xr_ar_space/
├── CMakeLists.txt
├── extern/
│   ├── glad/                    # Vendored OpenGL 4.6 Core loader (GLAD2)
│   │   ├── include/glad/gl.h
│   │   ├── include/KHR/khrplatform.h
│   │   └── src/gl.c
│   └── stb/                     # stb_image.h for test textures
├── shaders/
│   ├── screen.vert/frag         # Virtual screen rendering + hover highlight
│   └── cursor.vert/frag         # Cursor overlay
├── tests/                       # (empty, tests not yet implemented)
└── src/
    ├── main.cpp                  # Entry point
    ├── app/
    │   ├── App.h/cpp             # GLFW window, main loop, input dispatch
    │   └── Config.h              # Runtime configuration constants
    ├── renderer/
    │   ├── Shader.h/cpp          # GLSL shader compile/link, uniform setters
    │   ├── Camera.h/cpp          # Perspective projection, yaw/pitch rotation
    │   ├── VirtualScreen.h/cpp   # Textured quad (VAO/VBO/EBO)
    │   └── Renderer.h/cpp        # GL state management, render pipeline
    ├── capture/
    │   ├── CaptureSource.h       # Capture source descriptor
    │   ├── CaptureTexture.h/cpp  # GPU texture from captured frames
    │   ├── FrameBuffer.h         # Frame buffering
    │   ├── WindowCapturer.h/cpp  # GDI PrintWindow capture (30 FPS)
    │   └── WindowEnumerator.h/cpp # EnumWindows window listing
    ├── hid/
    │   ├── ImuProtocol.h         # XREAL constants, packet format, CRC
    │   └── ImuReader.h/cpp       # Background thread, hidapi read loop, parsing
    ├── tracking/
    │   ├── SensorFusion.h        # Complementary filter (alpha=0.98)
    │   └── HeadTracker.h/cpp     # Thread-safe 3DoF orientation
    ├── layout/
    │   └── LayoutManager.h       # Arc/Grid/Stack/Single layout computation
    ├── interaction/
    │   └── Raycaster.h           # Mouse ray-quad intersection, screen picking
    ├── display/                  # (not yet implemented)
    └── util/
        ├── Log.h                 # Timestamped console logging
        ├── Timer.h               # Frame delta timing
        └── MathUtils.h           # deg/rad, lerp, clamp
```

## Implementation Status

### Phase 1: Scaffold + Render Loop — COMPLETE

- [x] CMake project with GLFW, GLAD, GLM via FetchContent
- [x] `App` class with GLFW window, main loop, input handling
- [x] `Renderer` with OpenGL state management
- [x] `Shader` class (file loading, compile/link, uniform setters)
- [x] `Camera` with perspective projection and keyboard rotation
- [x] `VirtualScreen` textured quad (VAO/VBO/EBO)
- [x] GLSL shaders (screen + cursor)
- [x] Test scene: 3 checker-textured floating rectangles in an arc
- [x] Build verified — executable runs

### Phase 2: Window Capture — COMPLETE

- [x] `WindowEnumerator`: Window listing via EnumWindows
- [x] `WindowCapturer`: GDI PrintWindow capture at 30 FPS
- [x] `CaptureTexture`: GPU texture upload from captured frames
- [x] `FrameBuffer`: Frame buffering between capture and render threads
- [ ] **Planned upgrade**: WinRT GraphicsCapture + `WGL_NV_DX_interop2` for zero-copy GPU path

### Phase 3: USB HID + Head Tracking — COMPLETE

- [x] `ImuProtocol`: XREAL constants (VID `0x3318`, PIDs, enable command, Adler-32 CRC)
- [x] `ImuReader`: Background thread, hidapi read loop, 64-byte packet parsing
- [x] `SensorFusion`: Complementary filter (alpha=0.98), gyro+accel pitch/roll, pure gyro yaw
- [x] `HeadTracker`: Thread-safe orientation output

### Phase 4: Layout System — COMPLETE

- [x] `LayoutManager`: Arc, Grid, Stack, Single layout computation
- [ ] Animated transitions (lerp/slerp over ~300ms)

### Phase 5: Interaction — COMPLETE

- [x] `Raycaster`: Mouse unproject to world ray, ray-quad intersection
- [x] Drag-to-reposition selected screens
- [x] Scroll wheel zoom, double-click zoom
- [x] Visual feedback: hover highlight in shader

### Phase 6: Display Detection + Polish — NOT STARTED

- [ ] `DisplayDetector`: EnumDisplayDevices + EDID matching for XREAL
- [ ] `WindowPositioner`: Borderless fullscreen on XREAL display
- [ ] Config file (JSON), persistent settings
- [ ] Error handling (device disconnect, window close recovery)
- [ ] Click injection (forward mouse events to captured windows)

## Architecture

### Capture Pipeline (current)
```
GDI PrintWindow -> CPU bitmap -> glTexSubImage2D -> GLuint texture -> VirtualScreen quad
```

### Capture Pipeline (planned zero-copy upgrade)
```
WinRT GraphicsCapture -> ID3D11Texture2D -> WGL_NV_DX_interop2 -> GLuint texture -> VirtualScreen quad
```

### IMU Pipeline
```
ImuReader thread -> HeadTracker -> SensorFusion -> Camera orientation
```

### XREAL Hardware Constants
- Vendor ID: `0x3318`
- Product IDs: `0x0424` / `0x0428` / `0x0432` / `0x0426`
- IMU: USB HID Interface 3 (Interface 2 for Ultra)
- Packet: 64 bytes — header, timestamp(ns), gyro(3x24-bit), accel(3x24-bit), mag(3x16-bit)
- Data rate: 60-100 Hz

## Next Steps

1. **Phase 6: Display Detection** — auto-detect XREAL glasses via EDID, fullscreen output
2. **Click injection** — forward mouse clicks to captured windows for full interactivity
3. **WinRT capture upgrade** — zero-copy GPU path for lower latency
4. **Animated layout transitions** — smooth lerp/slerp when switching layouts
5. **Config persistence** — JSON settings file for user preferences
6. **Tests** — unit tests for sensor fusion, layouts, raycasting

## References

- [robmikh/Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample) — WinRT capture reference
- [WGL_NV_DX_interop2 gist](https://gist.github.com/mmozeiko/c99f9891ce723234854f0919bfd88eae) — D3D11-to-OpenGL interop
- [badicsalex/ar-drivers-rs](https://github.com/badicsalex/ar-drivers-rs) — XREAL IMU protocol reference

## License

Private project — not licensed for redistribution.
