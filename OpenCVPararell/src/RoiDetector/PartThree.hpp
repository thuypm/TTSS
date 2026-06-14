#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "RoiDetector/Common.hpp"
#include "Types.hpp"

namespace pmt {

std::vector<std::vector<std::vector<std::string>>> detectPartThree(
    const cv::Mat& gray,
    const std::vector<std::vector<std::map<std::string, BubbleBox>>>& groups,
    PickOptions options
);

} // namespace pmt
