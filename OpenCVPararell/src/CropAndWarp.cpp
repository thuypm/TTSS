#include "CropAndWarp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace pmt {

using std::runtime_error;
using std::vector;

namespace {

double distanceBetween(const cv::Point2f& a, const cv::Point2f& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

std::array<cv::Point2f, 4> reorderQuad(vector<cv::Point2f> points) {
    std::sort(points.begin(), points.end(), [](const cv::Point2f& a, const cv::Point2f& b) {
        return a.y < b.y;
    });

    vector<cv::Point2f> top{points[0], points[1]};
    vector<cv::Point2f> bottom{points[2], points[3]};
    std::sort(top.begin(), top.end(), [](const cv::Point2f& a, const cv::Point2f& b) {
        return a.x < b.x;
    });
    std::sort(bottom.begin(), bottom.end(), [](const cv::Point2f& a, const cv::Point2f& b) {
        return a.x < b.x;
    });

    return {top[0], top[1], bottom[1], bottom[0]};
}

bool isReasonableQuad(const std::array<cv::Point2f, 4>& quad, int width, int height) {
    const auto& [tl, tr, br, bl] = quad;
    const double topWidth = distanceBetween(tl, tr);
    const double bottomWidth = distanceBetween(bl, br);
    const double leftHeight = distanceBetween(tl, bl);
    const double rightHeight = distanceBetween(tr, br);

    const double minWidth = std::min(topWidth, bottomWidth);
    const double minHeight = std::min(leftHeight, rightHeight);
    if (minWidth < width * 0.35 || minHeight < height * 0.35) {
        return false;
    }

    const double widthRatio = std::max(topWidth, bottomWidth) / std::max(1.0, minWidth);
    const double heightRatio = std::max(leftHeight, rightHeight) / std::max(1.0, minHeight);
    return widthRatio < 1.8 && heightRatio < 1.8;
}

cv::Mat preprocessForPageDetection(const cv::Mat& source) {
    cv::Mat gray;
    if (source.channels() == 1) {
        gray = source.clone();
    } else {
        cv::cvtColor(source, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    cv::Mat edges;
    cv::Canny(blurred, edges, 50, 150);

    cv::Mat closed;
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(edges, closed, cv::MORPH_CLOSE, kernel);
    return closed;
}

std::array<cv::Point2f, 4> findPageQuad(const cv::Mat& source) {
    cv::Mat binary = preprocessForPageDetection(source);

    vector<vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double bestArea = 0.0;
    std::array<cv::Point2f, 4> bestQuad{};
    bool found = false;

    for (const auto& contour : contours) {
        const double perimeter = cv::arcLength(contour, true);
        vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, 0.02 * perimeter, true);
        if (approx.size() != 4) {
            continue;
        }

        vector<cv::Point2f> points;
        points.reserve(4);
        for (const cv::Point& point : approx) {
            points.emplace_back(static_cast<float>(point.x), static_cast<float>(point.y));
        }

        const auto quad = reorderQuad(points);
        const double area = std::abs(cv::contourArea(approx));
        if (area > bestArea && isReasonableQuad(quad, binary.cols, binary.rows)) {
            bestArea = area;
            bestQuad = quad;
            found = true;
        }
    }

    if (!found) {
        throw runtime_error("Cannot find page rectangle for perspective warp");
    }

    return bestQuad;
}

cv::Mat warpFromCorners(
    const cv::Mat& source,
    const std::array<cv::Point2f, 4>& corners,
    const cv::Size& outputSize
) {
    const std::array<cv::Point2f, 4> destination{
        cv::Point2f(0.0f, 0.0f),
        cv::Point2f(static_cast<float>(outputSize.width - 1), 0.0f),
        cv::Point2f(static_cast<float>(outputSize.width - 1), static_cast<float>(outputSize.height - 1)),
        cv::Point2f(0.0f, static_cast<float>(outputSize.height - 1))
    };

    const cv::Mat transform = cv::getPerspectiveTransform(corners, destination);
    cv::Mat warped;
    cv::warpPerspective(source, warped, transform, outputSize);
    return warped;
}

} // namespace

cv::Mat cropAndWarp(const cv::Mat& source, const PaperConfig& config) {
    if (source.empty()) {
        throw runtime_error("Empty source image");
    }
    if (config.width <= 0 || config.height <= 0) {
        throw runtime_error("Invalid paper size in config");
    }

    const auto pageQuad = findPageQuad(source);
    return warpFromCorners(source, pageQuad, cv::Size(config.width, config.height));
}

} // namespace pmt
