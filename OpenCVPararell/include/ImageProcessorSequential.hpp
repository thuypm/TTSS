#pragma once

#include <filesystem>
#include <vector>

#include "ImageProcessor.hpp"
#include "Types.hpp"

namespace pmt {

ImageProcessResult processOneImageSequential(
    const ImageProcessJob& job,
    const PaperConfig& config,
    const OutputPaths& output
);
std::vector<ImageProcessResult> processImageBatchSequential(
    const std::vector<std::filesystem::path>& files,
    const PaperConfig& config,
    const OutputPaths& output
);

} // namespace pmt
