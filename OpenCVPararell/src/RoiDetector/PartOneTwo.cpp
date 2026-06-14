#include "RoiDetector/PartOneTwo.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace pmt {

using std::max;
using std::string;
using std::vector;

namespace {

vector<vector<string>> detectPart(
    const cv::Mat& gray,
    const vector<std::map<string, BubbleBox>>& groups,
    PickOptions options
) {
    vector<vector<ScoreEntry>> scoresMap;
    vector<double> allRatios;
    scoresMap.reserve(groups.size());

    for (const auto& group : groups) {
        vector<ScoreEntry> scores = scoreBubbleGroup(gray, group);
        for (const auto& score : scores) {
            allRatios.push_back(score.ratio);
        }
        scoresMap.push_back(std::move(scores));
    }

    const AdaptiveThreshold adaptive = computeAdaptiveFillThresholdFromRatios(allRatios);
    if (adaptive.valid) {
        options.minScore = max(options.minScore, adaptive.filledRatio);
        options.partFillHighRef = adaptive.highRef;
    }

    vector<vector<string>> result;
    result.reserve(scoresMap.size());
    for (const auto& scores : scoresMap) {
        result.push_back(pickWinningLabelsFromGrayScores(scores, options));
    }

    return result;
}

} // namespace

vector<vector<string>> detectPartOne(
    const cv::Mat& gray,
    const vector<std::map<string, BubbleBox>>& groups,
    PickOptions options
) {
    return detectPart(gray, groups, options);
}

vector<vector<string>> detectPartTwo(
    const cv::Mat& gray,
    const vector<std::map<string, BubbleBox>>& groups,
    PickOptions options
) {
    return detectPart(gray, groups, options);
}

} // namespace pmt
