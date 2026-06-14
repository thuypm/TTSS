#pragma once

#include <opencv2/core.hpp>

#include "Types.hpp"

namespace pmt {

cv::Mat cropAndWarp(const cv::Mat& source, const PaperConfig& config);

} // namespace pmt
