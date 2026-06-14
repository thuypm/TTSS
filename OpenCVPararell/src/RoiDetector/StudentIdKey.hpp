#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "Types.hpp"

namespace pmt {

std::vector<std::vector<std::string>> detectStudentId(
    const cv::Mat& gray,
    const std::vector<std::map<std::string, BubbleBox>>& groups
);

std::vector<std::vector<std::string>> detectKey(
    const cv::Mat& gray,
    const std::vector<std::map<std::string, BubbleBox>>& groups
);

} // namespace pmt
