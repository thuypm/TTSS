#include "Config.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace pmt {

using std::filesystem::exists;
using std::filesystem::path;
using std::runtime_error;
using std::string;
using std::tolower;
using std::transform;

PaperConfig loadPaperConfig(const path& configPath) {
    if (!exists(configPath)) {
        throw runtime_error("Config file not found: " + configPath.string());
    }

    // TODO: Parse paper-config.json into ROI boxes when the Electron config shape is finalized.
    PaperConfig config;
    config.width = 0;
    config.height = 0;
    return config;
}

Backend parseBackend(const string& value) {
    string normalized = value;
    transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });

    if (normalized == "cpu") {
        return Backend::Cpu;
    }
    if (normalized == "cuda") {
        return Backend::Cuda;
    }

    throw runtime_error("Unsupported backend: " + value);
}

} // namespace pmt
