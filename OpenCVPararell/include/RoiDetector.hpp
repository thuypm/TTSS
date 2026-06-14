#pragma once

#include <vector>

#include <opencv2/core.hpp>

#include "Types.hpp"

namespace pmt {

struct AdaptiveThreshold {
    double filledRatio = 0.35;
    double minGap = 0.08;
};

double centerDarknessScore(const cv::Mat& gray, const BubbleBox& box);
AdaptiveThreshold computeAdaptiveFillThresholdFromRatios(const std::vector<double>& ratios);
ProcessedImageData detectRois(const cv::Mat& warped, const PaperConfig& config);

} // namespace pmt
