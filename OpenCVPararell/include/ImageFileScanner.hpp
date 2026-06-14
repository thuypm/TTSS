#pragma once

#include <filesystem>
#include <vector>

namespace pmt {

std::vector<std::filesystem::path> scanImageFiles(const std::filesystem::path& inputDir);

} // namespace pmt
