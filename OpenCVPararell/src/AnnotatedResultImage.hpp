#pragma once

#include <opencv2/core.hpp>

#include "Types.hpp"

namespace pmt {

cv::Mat drawDetectedBubblesOnImage(
    const cv::Mat& warped,
    const ProcessedImageData& data,
    const PaperConfig& config
);

} // namespace pmt
