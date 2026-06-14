#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace pmt {

enum class Backend {
    Cpu,
    Cuda
};

struct CliOptions {
    std::filesystem::path inputDir = "input";
    std::filesystem::path outputDir = "output";
    std::filesystem::path configPath = "samples/paper-config.json";
    int threads = 0;
    Backend backend = Backend::Cpu;
};

struct OutputPaths {
    std::filesystem::path root;
    std::filesystem::path warpedDir;
    std::filesystem::path annotatedDir;
    std::filesystem::path logsDir;
};

struct BubbleBox {
    cv::Point topLeft;
    cv::Point bottomRight;
};

struct PaperConfig {
    int width = 0;
    int height = 0;
    cv::Point score;
    cv::Point note;
    std::vector<std::map<std::string, BubbleBox>> studentId;
    std::vector<std::map<std::string, BubbleBox>> key;
    std::vector<std::map<std::string, BubbleBox>> partOne;
    std::vector<std::map<std::string, BubbleBox>> partTwo;
    std::vector<std::vector<std::map<std::string, BubbleBox>>> partThree;
};

struct ProcessedImageData {
    std::vector<std::vector<std::string>> studentId;
    std::vector<std::vector<std::string>> key;
    std::vector<std::vector<std::string>> partOne;
    std::vector<std::vector<std::string>> partTwo;
    std::vector<std::vector<std::vector<std::string>>> partThree;
    bool fallback = false;
};

struct ImageProcessResult {
    std::string sourcePath;
    std::string fileName;
    int pageNum = 0;
    bool success = true;
    std::string error;
    ProcessedImageData data;
    std::string warpedImagePath;
    std::string annotatedImagePath;
    double elapsedMs = 0.0;
};

} // namespace pmt
