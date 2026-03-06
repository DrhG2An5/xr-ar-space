#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <array>
#include <cstring>
#include <optional>

namespace xr {

struct ImuSample {
    uint64_t timestamp_us = 0; // microseconds
    glm::vec3 gyro{0.0f};     // rad/s (remapped axes)
    glm::vec3 accel{0.0f};    // m/s^2 (remapped axes)
};

namespace ImuProtocol {

// XREAL USB identifiers
inline constexpr uint16_t kVendorId = 0x3318;
inline constexpr uint16_t kPidAir      = 0x0424;
inline constexpr uint16_t kPidAir2     = 0x0428;
inline constexpr uint16_t kPidAir2Pro  = 0x0432;
inline constexpr uint16_t kPidUltra    = 0x0426;

inline constexpr int kImuInterfaceStandard = 3; // Air, Air 2, Air 2 Pro
inline constexpr int kImuInterfaceUltra    = 2; // Ultra

inline constexpr int kPacketSize  = 64;
inline constexpr uint8_t kMarker  = 0xAA;
inline constexpr uint8_t kCmdImu  = 0x19;

inline constexpr float kGravity   = 9.81f;
inline constexpr float kDegToRad  = 3.14159265358979323846f / 180.0f;

// Supported product IDs
inline bool isSupportedPid(uint16_t pid) {
    return pid == kPidAir || pid == kPidAir2 || pid == kPidAir2Pro || pid == kPidUltra;
}

// Get the IMU HID interface number for a given product ID
inline int imuInterface(uint16_t pid) {
    return (pid == kPidUltra) ? kImuInterfaceUltra : kImuInterfaceStandard;
}

// Adler-32 checksum (used in XREAL packet CRC field)
inline uint32_t adler32(const uint8_t* data, size_t len) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

// Read a signed 24-bit little-endian integer from a byte pointer
inline int32_t readI24LE(const uint8_t* p) {
    uint32_t val = static_cast<uint32_t>(p[0])
                 | (static_cast<uint32_t>(p[1]) << 8)
                 | (static_cast<uint32_t>(p[2]) << 16);
    // Sign-extend from 24 bits
    if (val & 0x800000) {
        val |= 0xFF000000;
    }
    return static_cast<int32_t>(val);
}

// Read an unsigned 16-bit little-endian value
inline uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

// Read an unsigned 32-bit little-endian value
inline uint32_t readU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

// Read an unsigned 64-bit little-endian value
inline uint64_t readU64LE(const uint8_t* p) {
    uint64_t lo = readU32LE(p);
    uint64_t hi = readU32LE(p + 4);
    return lo | (hi << 32);
}

// Build 64-byte HID command packet with header + CRC
// cmd_id: command identifier (e.g., 0x19 for IMU control)
// payload: command data bytes
inline std::array<uint8_t, kPacketSize> buildCommand(uint8_t cmdId,
                                                      const uint8_t* payload,
                                                      size_t payloadLen) {
    std::array<uint8_t, kPacketSize> pkt{};

    // Header
    pkt[0] = kMarker;
    // [1-4] CRC filled in below
    // [5-6] length = payloadLen + 3 (includes cmd_id + length field itself? per protocol)
    uint16_t length = static_cast<uint16_t>(payloadLen + 3);
    pkt[5] = static_cast<uint8_t>(length & 0xFF);
    pkt[6] = static_cast<uint8_t>((length >> 8) & 0xFF);
    // [7] cmd_id
    pkt[7] = cmdId;

    // Copy payload starting at byte 8
    if (payload && payloadLen > 0) {
        std::memcpy(&pkt[8], payload, payloadLen);
    }

    // Compute Adler-32 CRC over bytes [5..end_of_data]
    size_t crcStart = 5;
    size_t crcLen = 3 + payloadLen; // length(2) + cmd_id(1) + payload
    uint32_t crc = adler32(&pkt[crcStart], crcLen);
    pkt[1] = static_cast<uint8_t>(crc & 0xFF);
    pkt[2] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    pkt[3] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    pkt[4] = static_cast<uint8_t>((crc >> 24) & 0xFF);

    return pkt;
}

// Build the IMU enable command (cmd_id=0x19, data=[0x01])
inline std::array<uint8_t, kPacketSize> buildEnableCommand() {
    uint8_t payload[] = {0x01};
    return buildCommand(kCmdImu, payload, sizeof(payload));
}

// Build the IMU disable command (cmd_id=0x19, data=[0x00])
inline std::array<uint8_t, kPacketSize> buildDisableCommand() {
    uint8_t payload[] = {0x00};
    return buildCommand(kCmdImu, payload, sizeof(payload));
}

// Parse a 64-byte HID packet into an ImuSample.
// Returns std::nullopt if the packet is not a valid IMU report.
inline std::optional<ImuSample> parseImuReport(const uint8_t* pkt, size_t len) {
    if (len < kPacketSize) return std::nullopt;
    if (pkt[0] != kMarker) return std::nullopt;
    if (pkt[7] != kCmdImu) return std::nullopt;

    // IMU data starts at byte 8
    const uint8_t* d = pkt + 8;

    ImuSample sample;

    // Timestamp: bytes [4..11] from data start (u64 LE nanoseconds)
    sample.timestamp_us = readU64LE(d + 4) / 1000;

    // Gyroscope: mul(u16), div(u32), then 3x i24
    uint16_t gyroMul = readU16LE(d + 12);
    uint32_t gyroDiv = readU32LE(d + 14);
    if (gyroDiv == 0) return std::nullopt;

    int32_t rawGyroX = readI24LE(d + 18);
    int32_t rawGyroY = readI24LE(d + 21);
    int32_t rawGyroZ = readI24LE(d + 24);

    float gyroScale = static_cast<float>(gyroMul) / static_cast<float>(gyroDiv) * kDegToRad;
    // Axis remapping per ar-drivers-rs:
    // app_gyro.x = -raw_x, app_gyro.y = raw_z, app_gyro.z = raw_y
    sample.gyro.x = -static_cast<float>(rawGyroX) * gyroScale;
    sample.gyro.y =  static_cast<float>(rawGyroZ) * gyroScale;
    sample.gyro.z =  static_cast<float>(rawGyroY) * gyroScale;

    // Accelerometer: mul(u16), div(u32), then 3x i24
    uint16_t accelMul = readU16LE(d + 27);
    uint32_t accelDiv = readU32LE(d + 29);
    if (accelDiv == 0) return std::nullopt;

    int32_t rawAccelX = readI24LE(d + 33);
    int32_t rawAccelY = readI24LE(d + 36);
    int32_t rawAccelZ = readI24LE(d + 39);

    float accelScale = static_cast<float>(accelMul) / static_cast<float>(accelDiv) * kGravity;
    // Axis remapping:
    // app_accel.x = -raw_x, app_accel.y = raw_z, app_accel.z = raw_y
    sample.accel.x = -static_cast<float>(rawAccelX) * accelScale;
    sample.accel.y =  static_cast<float>(rawAccelZ) * accelScale;
    sample.accel.z =  static_cast<float>(rawAccelY) * accelScale;

    return sample;
}

} // namespace ImuProtocol
} // namespace xr
