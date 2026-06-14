#include "RoiDetector/StudentIdKey.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace pmt {

using std::max;
using std::string;
using std::vector;

namespace {

vector<vector<string>> detectDigitsByDarkest(
    const cv::Mat& gray,
    const vector<std::map<string, BubbleBox>>& groups,
    double minGap = 4.0
) {
    vector<vector<string>> result;
    result.reserve(groups.size());

    for (const auto& group : groups) {
        vector<std::pair<string, double>> candidates;
        for (const auto& [label, box] : group) {
            const int width = box.bottomRight.x - box.topLeft.x;
            const int height = box.bottomRight.y - box.topLeft.y;
            if (width <= 4 || height <= 4) {
                continue;
            }

            const int padX = max(1, static_cast<int>(std::round(width * 0.2)));
            const int padY = max(1, static_cast<int>(std::round(height * 0.2)));
            const cv::Rect bounds(0, 0, gray.cols, gray.rows);
            const cv::Rect roiRect(
                box.topLeft.x + padX,
                box.topLeft.y + padY,
                max(1, width - 2 * padX),
                max(1, height - 2 * padY)
            );
            const cv::Rect clipped = roiRect & bounds;
            if (clipped.empty()) {
                continue;
            }

            const double meanValue = cv::mean(gray(clipped))[0];
            candidates.push_back({label, 255.0 - meanValue});
        }

        if (candidates.empty()) {
            result.push_back({});
            continue;
        }

        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        if (candidates.size() >= 2 && candidates[0].second - candidates[1].second < minGap) {
            result.push_back({});
            continue;
        }

        result.push_back({candidates[0].first});
    }

    return result;
}

} // namespace

vector<vector<string>> detectStudentId(
    const cv::Mat& gray,
    const vector<std::map<string, BubbleBox>>& groups
) {
    return detectDigitsByDarkest(gray, groups);
}

vector<vector<string>> detectKey(
    const cv::Mat& gray,
    const vector<std::map<string, BubbleBox>>& groups
) {
    return detectDigitsByDarkest(gray, groups);
}

} // namespace pmt
