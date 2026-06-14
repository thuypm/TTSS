#include "RoiDetector.hpp"

#include <stdexcept>

#include <opencv2/imgproc.hpp>

#include "RoiDetector/Common.hpp"
#include "RoiDetector/PartOneTwo.hpp"
#include "RoiDetector/PartThree.hpp"
#include "RoiDetector/StudentIdKey.hpp"

namespace pmt {

using std::runtime_error;

ProcessedImageData detectRois(const cv::Mat& warped, const PaperConfig& config) {
    if (warped.empty()) {
        throw runtime_error("Empty warped image");
    }

    cv::Mat rawGray;
    if (warped.channels() == 1) {
        rawGray = warped;
    } else {
        cv::cvtColor(warped, rawGray, cv::COLOR_BGR2GRAY);
    }

    const cv::Mat grayForParts = prepareGrayForRecognition(warped);
    const AdaptiveThreshold sheetAdaptive = computeAdaptiveFillThresholdFromRatios(
        collectRatiosAllPartsForAdaptive(grayForParts, config)
    );
    const double sheetFillHighRef = sheetAdaptive.valid ? sheetAdaptive.highRef : -1.0;

    PickOptions partOneOptions;
    partOneOptions.sheetFillHighRef = sheetFillHighRef;

    PickOptions partTwoOptions;
    partTwoOptions.sheetFillHighRef = sheetFillHighRef;

    PickOptions partThreeOptions;
    partThreeOptions.tolerance = 0.035;
    partThreeOptions.minScore = 0.07;
    partThreeOptions.minSeparationFromRest = 0.028;
    partThreeOptions.singleWinnerOnly = true;
    partThreeOptions.sheetFillHighRef = sheetFillHighRef;

    ProcessedImageData data;
    data.studentId = detectStudentId(rawGray, config.studentId);
    data.key = detectKey(rawGray, config.key);
    data.partOne = detectPartOne(grayForParts, config.partOne, partOneOptions);
    data.partTwo = detectPartTwo(grayForParts, config.partTwo, partTwoOptions);
    data.partThree = detectPartThree(grayForParts, config.partThree, partThreeOptions);
    return data;
}

} // namespace pmt
