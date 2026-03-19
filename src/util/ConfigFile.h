#pragma once

#include "app/Config.h"
#include <string>

namespace xr {

class ConfigFile {
public:
    // Default config file path (next to executable)
    static constexpr const char* kDefaultFilename = "config.json";

    // Load config from a JSON file. Returns default Config if file doesn't exist.
    static Config load(const std::string& path = kDefaultFilename);

    // Save config to a JSON file.
    static bool save(const Config& config, const std::string& path = kDefaultFilename);

private:
    // Minimal JSON helpers (no external dependency)
    static std::string toJson(const Config& config);
    static Config fromJson(const std::string& json);

    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);

    // Simple JSON value extractors
    static bool findBool(const std::string& json, const std::string& key, bool defaultVal);
    static int findInt(const std::string& json, const std::string& key, int defaultVal);
    static float findFloat(const std::string& json, const std::string& key, float defaultVal);
    static std::string findString(const std::string& json, const std::string& key, const std::string& defaultVal);
};

} // namespace xr
