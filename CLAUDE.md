# CLAUDE.md — Project Instructions for Claude Code

## Project Overview

XREAL AR Multiscreen — native C++20 Windows application that streams desktop windows into XREAL AR glasses as interactive virtual 3D screens with real-time head tracking.

## Build Commands

```bash
# Configure
cmake -B build -G "Visual Studio 17 2022"

# Build
cmake --build build --config Release

# Run
./build/Release/xr_ar_space.exe
```

## Code Conventions

- **Language**: C++20, MSVC compiler
- **Build**: CMake 3.24+ with FetchContent for dependencies
- **Headers**: Use header-only implementations for small, self-contained modules (e.g., Raycaster.h, LayoutManager.h)
- **Naming**: PascalCase for classes/files, camelCase for methods/variables, UPPER_SNAKE for constants
- **Includes**: Use `#pragma once`, group includes: std lib -> third-party -> project headers
- **Error handling**: Use Log::error() for diagnostics, avoid exceptions in hot paths

## Architecture Rules

- **Module boundaries**: Each subdirectory under `src/` is a self-contained module (app, renderer, capture, hid, tracking, layout, interaction, display, util)
- **Threading**: Capture and IMU reading run on background threads. Use thread-safe handoff (ring buffers, mutexes) to the main render thread
- **Render loop**: Single-threaded OpenGL — all GL calls happen on the main thread only
- **Dependencies**: Added via CMake FetchContent (GLFW, GLM, hidapi). Do NOT add package managers like vcpkg or Conan
- **Vendored code**: `extern/glad/` and `extern/stb/` are vendored — do NOT modify or regenerate these

## File Locations

| What | Where |
|------|-------|
| Source code | `src/` |
| Shaders (GLSL) | `shaders/` |
| Vendored libs | `extern/glad/`, `extern/stb/` |
| Build output | `build/Release/` |
| Tests | `tests/` (not yet implemented) |

## Git Workflow

- **Create feature branches** for new work (e.g., `feature/display-detection`, `feature/winrt-capture`)
- **Commit frequently** with small, focused commits — easier to debug and revert
- **Never push to `master` directly** — use feature branches and merge
- **Never force-push** without explicit permission
- **Write descriptive commit messages** — summarize the "why", not just the "what"
- **Don't commit** build artifacts (`build/`), IDE files, or secrets

## Do NOT

- Add Node.js, Electron, or npm dependencies — this is a pure C++ project
- Modify vendored code in `extern/`
- Make OpenGL calls outside the main thread
- Add unnecessary abstractions — keep it simple, optimize later
- Commit without being asked
- Push without being asked

## XREAL Hardware Reference

- Vendor ID: `0x3318`
- Product IDs: `0x0424` / `0x0428` / `0x0432` / `0x0426`
- IMU: USB HID Interface 3 (Interface 2 for Ultra)
- IMU packet: 64 bytes, 60-100 Hz
- Sensor fusion: Complementary filter, alpha=0.98

## Current Status

- Phases 1-6 complete (rendering, capture, HID, tracking, layouts, interaction, display detection, config persistence)
- Keyboard forwarding via SendInput + SetForegroundWindow (works with all apps)
- Head tracking reconnection fully fixed (ref-counted HIDAPI, thread-safe device handle, timestamp guards)
- Per-window zoom with animated transitions (Ctrl+Z)
- Planned upgrades: WGL_NV_DX_interop2 zero-copy capture, SendInput mouse injection, 6DoF tracking
