#include "RoiDetector/PartThree.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace pmt {

using namespace std;


vector<vector<vector<string>>> detectPartThree(
    const cv::Mat& gray,
    const vector<vector<map<string, BubbleBox>>>& groups,
    PickOptions options
) {
    vector<vector<vector<ScoreEntry>>> scoresMap;
    vector<double> allRatios;
    scoresMap.reserve(groups.size());

    for (const auto& column : groups) {
        vector<vector<ScoreEntry>> columnScores;
        columnScores.reserve(column.size());
        for (const auto& group : column) {
            vector<ScoreEntry> scores = scoreBubbleGroup(gray, group);
            for (const auto& score : scores) {
                allRatios.push_back(score.ratio);
            }
            columnScores.push_back(move(scores));
        }
        scoresMap.push_back(move(columnScores));
    }

    const AdaptiveThreshold adaptive = computeAdaptiveFillThresholdFromRatios(allRatios);
    if (adaptive.valid) {
        options.minScore = max(options.minScore, adaptive.filledRatio);
        options.partFillHighRef = adaptive.highRef;
    }

    vector<vector<vector<string>>> result;
    result.reserve(groups.size());

    for (const auto& column : scoresMap) {
        vector<vector<string>> columnResult;
        columnResult.reserve(column.size());
        for (const auto& scores : column) {
            columnResult.push_back(pickWinningLabelsFromGrayScores(scores, options));
        }
        result.push_back(move(columnResult));
    }

    return result;
}

} // namespace pmt
