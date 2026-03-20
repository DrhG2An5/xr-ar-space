#pragma once

#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace xr {

// Abstract interface for capture sources.
// Implementations provide start/stop lifecycle and mutex-guarded frame access.
class CaptureSource {
public:
    virtual ~CaptureSource() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    // Lock the read buffer and return a pointer to BGRA pixel data.
    // Returns nullptr if no frame is available. Caller must call unlockFrame() after use.
    virtual const uint8_t* lockFrame(int& width, int& height) = 0;
    virtual void unlockFrame() = 0;

    // Get the source window handle (for click injection).
    virtual HWND hwnd() const = 0;
};

} // namespace xr
