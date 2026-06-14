#include "RoiDetector.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace pmt {

using std::accumulate;
using std::max;
using std::min;
using std::minmax_element;
using std::runtime_error;
using std::vector;

double centerDarknessScore(const cv::Mat& gray, const BubbleBox& box) {
    if (gray.empty()) {
        throw runtime_error("Empty grayscale image");
    }

    const cv::Rect imageBounds(0, 0, gray.cols, gray.rows);
    const cv::Rect roiRect(box.topLeft, box.bottomRight);
    const cv::Rect clipped = roiRect & imageBounds;
    if (clipped.empty()) {
        return 0.0;
    }

    const cv::Mat roi = gray(clipped);
    cv::Mat mask = cv::Mat::zeros(roi.size(), CV_8UC1);
    const cv::Point center(roi.cols / 2, roi.rows / 2);
    const int radius = max(1, min(roi.cols, roi.rows) / 3);
    cv::circle(mask, center, radius, cv::Scalar(255), cv::FILLED);

    const double meanValue = cv::mean(roi, mask)[0];
    return (255.0 - meanValue) / 255.0;
}

AdaptiveThreshold computeAdaptiveFillThresholdFromRatios(const vector<double>& ratios) {
    if (ratios.empty()) {
        return {};
    }

    const double sum = accumulate(ratios.begin(), ratios.end(), 0.0);
    const double mean = sum / static_cast<double>(ratios.size());
    const auto [minIt, maxIt] = minmax_element(ratios.begin(), ratios.end());

    AdaptiveThreshold threshold;
    threshold.filledRatio = max(0.25, mean + ((*maxIt - *minIt) * 0.25));
    threshold.minGap = 0.08;
    return threshold;
}

ProcessedImageData detectRois(const cv::Mat& warped, const PaperConfig& config) {
    (void)config;

    if (warped.empty()) {
        throw runtime_error("Empty warped image");
    }

    // TODO: Implement studentId/key/Part One/Two/Three recognition from configured ROI boxes.
    return {};
}

} // namespace pmt
