#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "RoiDetector.hpp"

namespace pmt {

struct ScoreEntry {
    std::string label;
    double ratio = 0.0;
};

struct PickOptions {
    double tolerance = 0.05;
    double minScore = 0.06;
    double minGap = 0.022;
    double minSeparationFromRest = 0.02;
    bool singleWinnerOnly = false;
    bool allowMultiFilledCluster = true;
    double multiFilledMaxFraction = 0.5;
    double multiFilledClusterRelax = 0.02;
    double partFillHighRef = -1.0;
    double sheetFillHighRef = -1.0;
    double maxBelowTypicalFillHighRef = 0.18;
};

cv::Mat prepareGrayForRecognition(const cv::Mat& image);
std::vector<ScoreEntry> scoreBubbleGroup(
    const cv::Mat& gray,
    const std::map<std::string, BubbleBox>& bubbleGroup
);
std::vector<std::string> pickWinningLabelsFromGrayScores(
    std::vector<ScoreEntry> scores,
    const PickOptions& options
);
std::vector<double> collectRatiosAllPartsForAdaptive(const cv::Mat& gray, const PaperConfig& config);

} // namespace pmt
