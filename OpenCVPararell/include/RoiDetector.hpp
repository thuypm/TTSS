#pragma once

#include <vector>

#include <opencv2/core.hpp>

#include "Types.hpp"

namespace pmt {

struct AdaptiveThreshold {
    double filledRatio = 0.35;
    double minGap = 0.08;
    double lowRef = 0.0;
    double highRef = 0.0;
    double spread = 0.0;
    int n = 0;
    bool valid = false;
};

double centerDarknessScore(const cv::Mat& gray, const BubbleBox& box);
AdaptiveThreshold computeAdaptiveFillThresholdFromRatios(const std::vector<double>& ratios);
ProcessedImageData detectRois(const cv::Mat& warped, const PaperConfig& config);

} // namespace pmt
