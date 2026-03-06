# XREAL AR Multiscreen

Native C++ application that streams multiple desktop windows into XREAL AR glasses as interactive virtual screens in 3D, with real-time 3DoF head tracking.

Rebuilt from an Electron/TypeScript prototype for lower latency, direct USB HID access, and tighter GPU control.

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Graphics | OpenGL 4.6 Core (GLFW + GLAD + GLM) |
| Window Capture | Windows.Graphics.Capture (WinRT) + D3D11-to-OpenGL interop |
| USB HID | hidapi (winapi backend) |
| Build | CMake 3.24+ with FetchContent |
| Language | C++20, MSVC |
| Platform | Windows 10/11 |

## Prerequisites

- **CMake 3.24+**
- **Visual Studio 2022 Build Tools** (MSVC C++ toolchain)
- **GPU with OpenGL 4.6 support** (any modern NVIDIA/AMD/Intel)

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

## Build

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

## Controls (Phase 1)

| Key | Action |
|-----|--------|
| Arrow Left/Right | Rotate camera yaw |
| Arrow Up/Down | Rotate camera pitch |
| Tab | Cycle selected screen |
| R | Reset camera orientation |
| Escape | Exit |

## Project Structure

```
xr_ar_space/
├── CMakeLists.txt
├── extern/
│   ├── glad/              # Vendored OpenGL 4.6 Core loader (GLAD2)
│   │   ├── include/glad/gl.h
│   │   ├── include/KHR/khrplatform.h
│   │   └── src/gl.c
│   └── stb/               # stb_image.h for test textures
├── shaders/
│   ├── screen.vert/frag   # Virtual screen rendering + selection border
│   └── cursor.vert/frag   # Cursor overlay (future use)
└── src/
    ├── main.cpp            # Entry point
    ├── app/
    │   ├── App.h/cpp       # GLFW window, main loop, input dispatch
    │   └── Config.h        # Runtime configuration constants
    ├── renderer/
    │   ├── Shader.h/cpp    # GLSL shader compile/link, uniform setters
    │   ├── Camera.h/cpp    # Perspective projection, yaw/pitch rotation
    │   ├── VirtualScreen.h/cpp  # Textured quad (VAO/VBO/EBO)
    │   └── Renderer.h/cpp  # GL state management, render pipeline
    ├── capture/            # Phase 2: WinRT window capture
    ├── hid/                # Phase 3: XREAL USB HID communication
    ├── tracking/           # Phase 3: IMU sensor fusion
    ├── layout/             # Phase 4: Arc/Grid/Stack/Single layouts
    ├── interaction/        # Phase 5: Raycasting, drag, zoom
    ├── display/            # Phase 6: XREAL display detection
    └── util/
        ├── Log.h           # Timestamped console logging
        ├── Timer.h         # Frame delta timing
        └── MathUtils.h     # deg/rad, lerp, clamp
```

## Implementation Status

### Phase 1: Scaffold + Render Loop — CODE COMPLETE, awaiting build verification

- [x] CMake project with GLFW, GLAD, GLM via FetchContent
- [x] `App` class with GLFW window, main loop, input handling
- [x] `Renderer` with OpenGL state management
- [x] `Shader` class (file loading, compile/link, uniform setters)
- [x] `Camera` with perspective projection and keyboard rotation
- [x] `VirtualScreen` textured quad (VAO/VBO/EBO)
- [x] GLSL shaders (screen + cursor)
- [x] Test scene: 3 checker-textured floating rectangles in an arc
- [ ] **Build verification** (blocked on CMake + MSVC installation)

### Phase 2: Window Capture — not started

- [ ] `DxGlInterop`: D3D11 device + `WGL_NV_DX_interop2` for zero-copy GPU texture sharing
- [ ] `CaptureSession`: WinRT GraphicsCaptureItem, FramePool, FrameArrived callback
- [ ] `CaptureManager`: Window enumeration (EnumWindows), session lifecycle
- [ ] CPU fallback path (Map + glTexSubImage2D) if WGL interop unavailable

### Phase 3: USB HID + Head Tracking — not started

- [ ] `ImuProtocol`: XREAL constants (VID `0x3318`, PIDs, enable command, Adler-32 CRC)
- [ ] `ImuReader`: Background thread, hidapi read loop, 64-byte packet parsing
- [ ] `SensorFusion`: Complementary filter (alpha=0.98), gyro+accel pitch/roll, pure gyro yaw
- [ ] `HeadTracker`: Thread-safe orientation via lock-free SPSC ring buffer

### Phase 4: Layout System — not started

- [ ] `ILayout` interface with `compute(screens, params)`
- [ ] Arc, Grid, Stack, Single layout implementations
- [ ] Animated transitions (lerp/slerp over ~300ms)
- [ ] Keyboard shortcuts (1-4 to switch, Tab to cycle focus)

### Phase 5: Interaction — not started

- [ ] `Raycaster`: Mouse unproject to world ray, ray-quad intersection
- [ ] `ScreenDragger`: Drag-to-reposition selected screens
- [ ] Double-click zoom, scroll wheel distance adjustment
- [ ] Visual feedback: highlight borders, selection indicators

### Phase 6: Display Detection + Polish — not started

- [ ] `DisplayDetector`: EnumDisplayDevices + EDID matching for XREAL
- [ ] `WindowPositioner`: Borderless fullscreen on XREAL display
- [ ] Config file (JSON), persistent settings
- [ ] Error handling (device disconnect, window close recovery)

## Architecture Notes

### Capture Pipeline (Phase 2, zero-copy path)
```
WinRT GraphicsCapture -> ID3D11Texture2D -> WGL_NV_DX_interop2 -> GLuint texture -> VirtualScreen quad
```

### IMU Pipeline (Phase 3)
```
ImuReader thread -> SPSC ring buffer -> HeadTracker (main thread) -> SensorFusion -> Camera
```

### XREAL Hardware Constants
- Vendor ID: `0x3318`
- Product IDs: `0x0424` / `0x0428` / `0x0432` / `0x0426`
- IMU: USB HID Interface 3 (Interface 2 for Ultra)
- Packet: 64 bytes — header, timestamp(ns), gyro(3x24-bit), accel(3x24-bit), mag(3x16-bit)
- Data rate: 60-100 Hz

## Next Steps

1. **Install build tools** (CMake + MSVC) — see Prerequisites above
2. **Build and run Phase 1** — verify the 3 checker-textured quads render with camera rotation
3. **Phase 2: Window Capture** — the most technically challenging phase (WinRT/COM interop, threading)
4. **Phase 3: Head Tracking** — direct USB HID with hidapi, sensor fusion

## References

- [robmikh/Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample) — WinRT capture reference
- [WGL_NV_DX_interop2 gist](https://gist.github.com/mmozeiko/c99f9891ce723234854f0919bfd88eae) — D3D11-to-OpenGL interop
- [badicsalex/ar-drivers-rs](https://github.com/badicsalex/ar-drivers-rs) — XREAL IMU protocol reference

## License

Private project — not licensed for redistribution.
