#include "util/ConfigFile.h"
#include "util/Log.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace xr {

std::string ConfigFile::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool ConfigFile::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return file.good();
}

// Simple JSON value extraction — no external JSON library needed.
// Handles flat key-value pairs in a single-level JSON object.

static std::string::size_type findKey(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return std::string::npos;
    // Advance past the key and the colon
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return std::string::npos;
    return pos + 1;
}

bool ConfigFile::findBool(const std::string& json, const std::string& key, bool defaultVal) {
    auto pos = findKey(json, key);
    if (pos == std::string::npos) return defaultVal;
    auto sub = json.substr(pos, 10);
    if (sub.find("true") != std::string::npos) return true;
    if (sub.find("false") != std::string::npos) return false;
    return defaultVal;
}

int ConfigFile::findInt(const std::string& json, const std::string& key, int defaultVal) {
    auto pos = findKey(json, key);
    if (pos == std::string::npos) return defaultVal;
    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try {
        return std::stoi(json.substr(pos));
    } catch (...) {
        return defaultVal;
    }
}

float ConfigFile::findFloat(const std::string& json, const std::string& key, float defaultVal) {
    auto pos = findKey(json, key);
    if (pos == std::string::npos) return defaultVal;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try {
        return std::stof(json.substr(pos));
    } catch (...) {
        return defaultVal;
    }
}

std::string ConfigFile::findString(const std::string& json, const std::string& key, const std::string& defaultVal) {
    auto pos = findKey(json, key);
    if (pos == std::string::npos) return defaultVal;
    auto start = json.find('"', pos);
    if (start == std::string::npos) return defaultVal;
    ++start;
    auto end = json.find('"', start);
    if (end == std::string::npos) return defaultVal;
    return json.substr(start, end - start);
}

std::string ConfigFile::toJson(const Config& config) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"windowWidth\": " << config.windowWidth << ",\n";
    ss << "  \"windowHeight\": " << config.windowHeight << ",\n";
    ss << "  \"windowTitle\": \"" << config.windowTitle << "\",\n";
    ss << "  \"fullscreen\": " << (config.fullscreen ? "true" : "false") << ",\n";
    ss << "  \"fovDegrees\": " << config.fovDegrees << ",\n";
    ss << "  \"nearPlane\": " << config.nearPlane << ",\n";
    ss << "  \"farPlane\": " << config.farPlane << ",\n";
    ss << "  \"screenDistance\": " << config.screenDistance << ",\n";
    ss << "  \"screenWidth\": " << config.screenWidth << ",\n";
    ss << "  \"screenHeight\": " << config.screenHeight << ",\n";
    ss << "  \"rotationSpeed\": " << config.rotationSpeed << ",\n";
    ss << "  \"arcSpanDeg\": " << config.arcSpanDeg << ",\n";
    ss << "  \"gridGap\": " << config.gridGap << ",\n";
    ss << "  \"stackZOffset\": " << config.stackZOffset << ",\n";
    ss << "  \"minDistance\": " << config.minDistance << ",\n";
    ss << "  \"maxDistance\": " << config.maxDistance << ",\n";
    ss << "  \"scrollSpeed\": " << config.scrollSpeed << ",\n";
    ss << "  \"captureFrameRate\": " << config.captureFrameRate << ",\n";
    ss << "  \"shaderPath\": \"" << config.shaderPath << "\"\n";
    ss << "}\n";
    return ss.str();
}

Config ConfigFile::fromJson(const std::string& json) {
    Config c;
    c.windowWidth     = findInt(json, "windowWidth", c.windowWidth);
    c.windowHeight    = findInt(json, "windowHeight", c.windowHeight);
    c.windowTitle     = findString(json, "windowTitle", c.windowTitle);
    c.fullscreen      = findBool(json, "fullscreen", c.fullscreen);
    c.fovDegrees      = findFloat(json, "fovDegrees", c.fovDegrees);
    c.nearPlane       = findFloat(json, "nearPlane", c.nearPlane);
    c.farPlane        = findFloat(json, "farPlane", c.farPlane);
    c.screenDistance   = findFloat(json, "screenDistance", c.screenDistance);
    c.screenWidth     = findFloat(json, "screenWidth", c.screenWidth);
    c.screenHeight    = findFloat(json, "screenHeight", c.screenHeight);
    c.rotationSpeed   = findFloat(json, "rotationSpeed", c.rotationSpeed);
    c.arcSpanDeg      = findFloat(json, "arcSpanDeg", c.arcSpanDeg);
    c.gridGap         = findFloat(json, "gridGap", c.gridGap);
    c.stackZOffset    = findFloat(json, "stackZOffset", c.stackZOffset);
    c.minDistance      = findFloat(json, "minDistance", c.minDistance);
    c.maxDistance      = findFloat(json, "maxDistance", c.maxDistance);
    c.scrollSpeed     = findFloat(json, "scrollSpeed", c.scrollSpeed);
    c.captureFrameRate = findInt(json, "captureFrameRate", c.captureFrameRate);
    c.shaderPath      = findString(json, "shaderPath", c.shaderPath);
    return c;
}

Config ConfigFile::load(const std::string& path) {
    std::string json = readFile(path);
    if (json.empty()) {
        Log::info("No config file found at '{}' — using defaults", path);
        return {};
    }
    Log::info("Loaded config from '{}'", path);
    return fromJson(json);
}

bool ConfigFile::save(const Config& config, const std::string& path) {
    std::string json = toJson(config);
    if (!writeFile(path, json)) {
        Log::error("Failed to save config to '{}'", path);
        return false;
    }
    Log::info("Config saved to '{}'", path);
    return true;
}

} // namespace xr
