#pragma once

#include <filesystem>

#include <opencv2/core.hpp>

#include "Types.hpp"

namespace pmt {

std::filesystem::path saveAnnotatedImage(
    const cv::Mat& warped,
    const ProcessedImageData& data,
    const PaperConfig& config,
    const OutputPaths& output,
    const std::string& fileName
);

} // namespace pmt
