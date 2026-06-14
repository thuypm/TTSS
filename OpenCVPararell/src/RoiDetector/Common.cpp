#include "RoiDetector/Common.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace pmt {

using std::accumulate;
using std::max;
using std::min;
using std::runtime_error;
using std::size_t;
using std::string;
using std::vector;

double centerDarknessScore(const cv::Mat& gray, const BubbleBox& box) {
    if (gray.empty()) {
        throw runtime_error("Empty grayscale image");
    }

    const int width = box.bottomRight.x - box.topLeft.x;
    const int height = box.bottomRight.y - box.topLeft.y;
    if (width <= 2 || height <= 2) {
        return 0.0;
    }

    const int padX = max(1, static_cast<int>(std::round(width * 0.2)));
    const int padY = max(1, static_cast<int>(std::round(height * 0.2)));
    const cv::Rect imageBounds(0, 0, gray.cols, gray.rows);
    const cv::Rect roiRect(
        box.topLeft.x + padX,
        box.topLeft.y + padY,
        max(1, width - 2 * padX),
        max(1, height - 2 * padY)
    );
    const cv::Rect clipped = roiRect & imageBounds;
    if (clipped.empty()) {
        return 0.0;
    }

    const cv::Mat roi = gray(clipped);
    cv::Mat mask = cv::Mat::zeros(roi.size(), CV_8UC1);
    const cv::Point center(roi.cols / 2, roi.rows / 2);
    const int radius = max(2, static_cast<int>(std::round(min(roi.cols, roi.rows) * 0.35)));
    cv::circle(mask, center, radius, cv::Scalar(255), cv::FILLED);

    const double meanValue = cv::mean(roi, mask)[0];
    return (255.0 - meanValue) / 255.0;
}

AdaptiveThreshold computeAdaptiveFillThresholdFromRatios(const vector<double>& ratios) {
    if (ratios.size() < 4) {
        return {};
    }

    vector<double> sorted = ratios;
    std::sort(sorted.begin(), sorted.end());
    const int n = static_cast<int>(sorted.size());
    const int minHighTailCount = 4;
    const double minUpperFraction = 0.02;
    const double maxUpperFraction = 0.7;
    const double minSpread = 0.012;
    const double relaxDownAbs = 0.02;

    if (n >= 6) {
        double bestGap = -1.0;
        int bestIndex = -1;
        const int minUpperCount = max(minHighTailCount, static_cast<int>(std::ceil(n * minUpperFraction)));
        const int maxUpperCount = max(minUpperCount + 1, static_cast<int>(std::floor(n * maxUpperFraction)));

        for (int i = 0; i < n - 1; ++i) {
            const double gap = sorted[i + 1] - sorted[i];
            const int upperCount = n - (i + 1);
            if (!std::isfinite(gap) || gap <= 0.0) {
                continue;
            }
            if (upperCount < minUpperCount || upperCount > maxUpperCount) {
                continue;
            }
            if (gap > bestGap) {
                bestGap = gap;
                bestIndex = i;
            }
        }

        if (bestIndex >= 0 && bestGap >= minSpread) {
            const double highSum = accumulate(sorted.begin() + bestIndex + 1, sorted.end(), 0.0);
            const int highCount = n - (bestIndex + 1);
            AdaptiveThreshold threshold;
            threshold.filledRatio = max(0.0, sorted[bestIndex + 1] - relaxDownAbs);
            threshold.lowRef = sorted[bestIndex];
            threshold.highRef = highCount > 0 ? highSum / highCount : sorted.back();
            threshold.spread = bestGap;
            threshold.n = n;
            threshold.valid = true;
            return threshold;
        }
    }

    AdaptiveThreshold threshold;
    const int lowHalfEnd = max(1, n / 2);
    const double lowSum = accumulate(sorted.begin(), sorted.begin() + lowHalfEnd, 0.0);
    const double lowRef = lowSum / lowHalfEnd;
    const int highCount = min(n, max(minHighTailCount, static_cast<int>(std::floor(n * 0.2))));
    const double highSum = accumulate(sorted.end() - highCount, sorted.end(), 0.0);
    const double highRef = highSum / highCount;
    const double spread = highRef - lowRef;
    if (!std::isfinite(spread) || spread < minSpread) {
        return threshold;
    }

    threshold.filledRatio = lowRef + spread * 0.55;
    threshold.lowRef = lowRef;
    threshold.highRef = highRef;
    threshold.spread = spread;
    threshold.n = n;
    threshold.valid = true;
    return threshold;
}

double medianSortedAsc(vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    if (values.size() % 2 == 1) {
        return values[middle];
    }
    return (values[middle - 1] + values[middle]) / 2.0;
}

vector<string> pickWinningLabelsFromGrayScores(vector<ScoreEntry> scores, const PickOptions& options) {
    if (scores.empty()) {
        return {};
    }

    std::sort(scores.begin(), scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return a.ratio > b.ratio;
    });

    const ScoreEntry& best = scores[0];
    if (best.ratio < options.minScore) {
        return {};
    }

    vector<double> highRefs;
    if (options.partFillHighRef >= 0.0 && std::isfinite(options.partFillHighRef)) {
        highRefs.push_back(options.partFillHighRef);
    }
    if (options.sheetFillHighRef >= 0.0 && std::isfinite(options.sheetFillHighRef)) {
        highRefs.push_back(options.sheetFillHighRef);
    }
    if (!highRefs.empty()) {
        const double typicalHigh = *std::max_element(highRefs.begin(), highRefs.end());
        if (best.ratio < typicalHigh - options.maxBelowTypicalFillHighRef) {
            return {};
        }
    }

    if (options.allowMultiFilledCluster) {
        const int maxMulti = static_cast<int>(std::floor(scores.size() * options.multiFilledMaxFraction));
        if (maxMulti >= 2) {
            const double clusterBand = options.tolerance + max(0.0, options.multiFilledClusterRelax);
            vector<string> cluster;
            for (const auto& score : scores) {
                if (score.ratio >= options.minScore && best.ratio - score.ratio <= clusterBand) {
                    cluster.push_back(score.label);
                }
            }
            if (cluster.size() >= 2) {
                if (static_cast<int>(cluster.size()) > maxMulti) {
                    return {};
                }
                return cluster;
            }
        }
    }

    if (scores.size() >= 2 && best.ratio - scores[1].ratio < options.minGap) {
        return {};
    }

    if (options.minSeparationFromRest >= 0.0 && scores.size() >= 2) {
        vector<double> restRatios;
        restRatios.reserve(scores.size() - 1);
        for (size_t i = 1; i < scores.size(); ++i) {
            restRatios.push_back(scores[i].ratio);
        }
        if (best.ratio - medianSortedAsc(restRatios) < options.minSeparationFromRest) {
            return {};
        }
    }

    vector<string> labels;
    for (const auto& score : scores) {
        if (best.ratio - score.ratio <= options.tolerance) {
            labels.push_back(score.label);
        }
    }

    if (options.singleWinnerOnly && labels.size() > 1) {
        return {};
    }

    return labels;
}

cv::Mat prepareGrayForRecognition(const cv::Mat& image) {
    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image.clone();
    } else {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat equalized;
    const cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(gray, equalized);
    return equalized;
}

vector<ScoreEntry> scoreBubbleGroup(
    const cv::Mat& gray,
    const std::map<string, BubbleBox>& bubbleGroup
) {
    vector<ScoreEntry> scores;
    scores.reserve(bubbleGroup.size());

    for (const auto& [label, box] : bubbleGroup) {
        scores.push_back({label, centerDarknessScore(gray, box)});
    }

    return scores;
}

vector<double> collectRatiosAllPartsForAdaptive(const cv::Mat& gray, const PaperConfig& config) {
    vector<double> ratios;
    const auto pushGroup = [&](const std::map<string, BubbleBox>& group) {
        for (const auto& [_, box] : group) {
            ratios.push_back(centerDarknessScore(gray, box));
        }
    };

    for (const auto& group : config.partOne) {
        pushGroup(group);
    }
    for (const auto& group : config.partTwo) {
        pushGroup(group);
    }
    for (const auto& column : config.partThree) {
        for (const auto& group : column) {
            pushGroup(group);
        }
    }

    return ratios;
}

} // namespace pmt
