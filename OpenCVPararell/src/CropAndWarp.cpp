#include "CropAndWarp.hpp"

#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace pmt {

using std::runtime_error;

cv::Mat cropAndWarp(const cv::Mat& source, const PaperConfig& config) {
    if (source.empty()) {
        throw runtime_error("Empty source image");
    }

    // TODO: Port contour/marker-based perspective detection from OpenCV.js.
    // For the initial project skeleton, keep the image unchanged so the CLI pipeline is testable.
    if (config.width > 0 && config.height > 0) {
        cv::Mat resized;
        cv::resize(source, resized, cv::Size(config.width, config.height));
        return resized;
    }

    return source.clone();
}

} // namespace pmt
