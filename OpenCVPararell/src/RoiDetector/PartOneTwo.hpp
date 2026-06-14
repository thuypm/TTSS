#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "RoiDetector/Common.hpp"
#include "Types.hpp"

namespace pmt {

std::vector<std::vector<std::string>> detectPartOne(
    const cv::Mat& gray,
    const std::vector<std::map<std::string, BubbleBox>>& groups,
    PickOptions options
);

std::vector<std::vector<std::string>> detectPartTwo(
    const cv::Mat& gray,
    const std::vector<std::map<std::string, BubbleBox>>& groups,
    PickOptions options
);

} // namespace pmt
