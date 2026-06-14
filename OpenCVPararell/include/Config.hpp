#pragma once

#include <filesystem>
#include <string>

#include "Types.hpp"

namespace pmt {

PaperConfig loadPaperConfig(const std::filesystem::path& configPath);
Backend parseBackend(const std::string& value);

} // namespace pmt
