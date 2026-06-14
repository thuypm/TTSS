#pragma once

#include <filesystem>
#include <vector>

#include "Types.hpp"

namespace pmt {

struct ImageProcessJob {
    std::filesystem::path imagePath;
    int pageNum = 0;
};

ImageProcessResult processOneImage(
    const ImageProcessJob& job,
    const PaperConfig& config,
    const OutputPaths& output
);
std::vector<ImageProcessResult> processImageBatch(
    const std::vector<std::filesystem::path>& files,
    const PaperConfig& config,
    const OutputPaths& output,
    int threads
);

} // namespace pmt
