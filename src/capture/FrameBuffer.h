#pragma once

#include <cstdint>
#include <vector>
#include <mutex>

namespace xr {

// Thread-safe double-buffered pixel storage for BGRA frames.
// Writer thread calls write() to copy pixels into the back buffer, then swaps.
// Reader thread calls lockRead()/unlockRead() to access the front buffer.
class FrameBuffer {
public:
    FrameBuffer() = default;

    // Called by the writer thread: copies BGRA pixel data into the back buffer,
    // then swaps front/back pointers under the lock.
    void write(const uint8_t* data, int width, int height) {
        int stride = width * 4;
        size_t size = static_cast<size_t>(stride) * height;

        // Resize back buffer outside the lock (allocation is slow)
        if (m_back.size() != size) {
            m_back.resize(size);
        }
        std::memcpy(m_back.data(), data, size);

        {
            std::lock_guard lock(m_mutex);
            std::swap(m_front, m_back);
            m_width = width;
            m_height = height;
            m_hasFrame = true;
        }
    }

    // Called by the reader thread: locks and returns pointer to front buffer.
    // Returns nullptr if no frame has been written yet.
    const uint8_t* lockRead(int& width, int& height) {
        m_mutex.lock();
        if (!m_hasFrame) {
            m_mutex.unlock();
            return nullptr;
        }
        width = m_width;
        height = m_height;
        return m_front.data();
    }

    void unlockRead() {
        m_mutex.unlock();
    }

private:
    std::mutex m_mutex;
    std::vector<uint8_t> m_front;
    std::vector<uint8_t> m_back;
    int m_width = 0;
    int m_height = 0;
    bool m_hasFrame = false;
};

} // namespace xr
