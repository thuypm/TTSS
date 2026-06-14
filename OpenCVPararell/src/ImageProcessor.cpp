#include "ImageProcessor.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#ifdef PMT_SCANNER_HAS_OPENMP
#include <omp.h>
#endif

#include "Annotator.hpp"
#include "CropAndWarp.hpp"
#include "RoiDetector.hpp"

namespace pmt {

using namespace std;

using std::filesystem::path;
using std::filesystem::relative;
using std::chrono::duration;
using std::chrono::steady_clock;

ImageProcessResult processOneImage(
    const ImageProcessJob& job,
    const PaperConfig& config,
    const OutputPaths& output
) {
    const auto start = steady_clock::now();

    ImageProcessResult result;
    result.sourcePath = job.imagePath.string();
    result.fileName = job.imagePath.filename().string();
    result.pageNum = job.pageNum;

    try {
        const cv::Mat source = cv::imread(job.imagePath.string(), cv::IMREAD_COLOR);
        if (source.empty()) {
            throw runtime_error("Cannot read image");
        }

        const cv::Mat warped = cropAndWarp(source, config);
        result.data = detectRois(warped, config);

        const auto warpedPath = output.warpedDir / result.fileName;
        if (!cv::imwrite(warpedPath.string(), warped)) {
            throw runtime_error("Cannot write warped image: " + warpedPath.string());
        }

        result.warpedImagePath = relative(warpedPath, output.root).string();
        result.annotatedImagePath = saveAnnotatedImage(warped, result.data, config, output, result.fileName).string();
        result.success = true;
    } catch (const exception& e) {
        result.success = false;
        result.data.fallback = true;
        result.error = e.what();
    }

    const auto end = steady_clock::now();
    result.elapsedMs = duration<double, milli>(end - start).count();
    return result;
}

vector<ImageProcessResult> processImageBatch(
    const vector<path>& files,
    const PaperConfig& config,
    const OutputPaths& output,
    int threads
) {
    vector<ImageProcessResult> results(files.size());
    if (files.empty()) {
        return results;
    }

#ifdef PMT_SCANNER_HAS_OPENMP
    const int batchThreads = min(max(1, threads), static_cast<int>(files.size()));
#pragma omp parallel for schedule(dynamic) num_threads(batchThreads) if(batchThreads > 1)
#else
    (void)threads;
#endif
    for (int i = 0; i < static_cast<int>(files.size()); ++i) {
        const ImageProcessJob job{files[i], i + 1};
        results[i] = processOneImage(job, config, output);
    }

    return results;
}

} // namespace pmt
