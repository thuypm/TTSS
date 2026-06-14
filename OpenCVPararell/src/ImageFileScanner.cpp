#include "ImageFileScanner.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace pmt {

using std::filesystem::directory_iterator;
using std::filesystem::exists;
using std::filesystem::is_directory;
using std::filesystem::path;
using std::runtime_error;
using std::sort;
using std::string;
using std::tolower;
using std::transform;
using std::vector;

namespace {

string toLower(string value) {
    transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return value;
}

bool isSupportedImageExtension(const path& filePath) {
    const auto ext = toLower(filePath.extension().string());
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
}

} // namespace

vector<path> scanImageFiles(const path& inputDir) {
    if (!exists(inputDir)) {
        throw runtime_error("Input directory not found: " + inputDir.string());
    }
    if (!is_directory(inputDir)) {
        throw runtime_error("Input path is not a directory: " + inputDir.string());
    }

    vector<path> files;

    for (const auto& entry : directory_iterator(inputDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (isSupportedImageExtension(entry.path())) {
            files.push_back(entry.path());
        }
    }

    sort(files.begin(), files.end());
    return files;
}

} // namespace pmt
