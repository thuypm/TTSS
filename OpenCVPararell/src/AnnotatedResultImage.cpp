#include "AnnotatedResultImage.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace pmt {

using std::map;
using std::string;
using std::vector;

namespace {

const cv::Scalar kBlue(255, 0, 0);
const cv::Scalar kRed(0, 0, 255);
const int kStrokeThickness = 2;
const int kSelectionBoxPadding = 4;

int circleRadiusFromRoi(const BubbleBox& box) {
    const int height = box.bottomRight.y - box.topLeft.y;
    return std::max(1, (height + kSelectionBoxPadding) / 2);
}

cv::Point circleCenterFromRoi(const BubbleBox& box) {
    return {
        (box.topLeft.x + box.bottomRight.x) / 2,
        (box.topLeft.y + box.bottomRight.y) / 2
    };
}

void drawCircleForBox(cv::Mat& canvas, const BubbleBox& box, const cv::Scalar& color) {
    cv::circle(
        canvas,
        circleCenterFromRoi(box),
        circleRadiusFromRoi(box),
        color,
        kStrokeThickness,
        cv::LINE_AA
    );
}

void drawSelectedGroup(
    cv::Mat& canvas,
    const map<string, BubbleBox>& boxes,
    const vector<string>& selectedLabels,
    const cv::Scalar& color
) {
    for (const string& label : selectedLabels) {
        const auto boxIt = boxes.find(label);
        if (boxIt != boxes.end()) {
            drawCircleForBox(canvas, boxIt->second, color);
        }
    }
}

void drawSelectedGroups(
    cv::Mat& canvas,
    const vector<map<string, BubbleBox>>& boxes,
    const vector<vector<string>>& selected,
    const cv::Scalar& color
) {
    const size_t count = std::min(boxes.size(), selected.size());
    for (size_t i = 0; i < count; ++i) {
        drawSelectedGroup(canvas, boxes[i], selected[i], color);
    }
}

void drawSelectedNestedGroups(
    cv::Mat& canvas,
    const vector<vector<map<string, BubbleBox>>>& boxes,
    const vector<vector<vector<string>>>& selected,
    const cv::Scalar& color
) {
    const size_t outerCount = std::min(boxes.size(), selected.size());
    for (size_t i = 0; i < outerCount; ++i) {
        const size_t innerCount = std::min(boxes[i].size(), selected[i].size());
        for (size_t j = 0; j < innerCount; ++j) {
            drawSelectedGroup(canvas, boxes[i][j], selected[i][j], color);
        }
    }
}

} // namespace

cv::Mat drawDetectedBubblesOnImage(
    const cv::Mat& warped,
    const ProcessedImageData& data,
    const PaperConfig& config
) {
    cv::Mat annotated;
    if (warped.channels() == 1) {
        cv::cvtColor(warped, annotated, cv::COLOR_GRAY2BGR);
    } else {
        annotated = warped.clone();
    }

    drawSelectedGroups(annotated, config.studentId, data.studentId, kBlue);
    drawSelectedGroups(annotated, config.key, data.key, kBlue);
    drawSelectedGroups(annotated, config.partOne, data.partOne, kRed);
    drawSelectedGroups(annotated, config.partTwo, data.partTwo, kRed);
    drawSelectedNestedGroups(annotated, config.partThree, data.partThree, kRed);

    return annotated;
}

} // namespace pmt
