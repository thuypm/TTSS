#pragma once

#include <filesystem>
#include <vector>

#include "Types.hpp"

namespace pmt {

OutputPaths prepareOutputPaths(const std::filesystem::path& outputDir);
void writeResultsJson(
    const std::filesystem::path& outputDir,
    const std::filesystem::path& inputDir,
    const std::vector<ImageProcessResult>& results,
    double elapsedMs
);
void writeResultsCsv(
    const std::filesystem::path& outputDir,
    const std::vector<ImageProcessResult>& results
);
void writeErrorLog(
    const std::filesystem::path& outputDir,
    const std::vector<ImageProcessResult>& results
);

} // namespace pmt
