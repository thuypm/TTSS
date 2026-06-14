#include "Annotator.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include "AnnotatedResultImage.hpp"

namespace pmt {

using namespace std;

using std::filesystem::path;
using std::filesystem::relative;

path saveAnnotatedImage(
    const cv::Mat& warped,
    const ProcessedImageData& data,
    const PaperConfig& config,
    const OutputPaths& output,
    const string& fileName
) {
    const auto annotatedPath = output.annotatedDir / fileName;
    const cv::Mat annotated = drawDetectedBubblesOnImage(warped, data, config);

    if (!cv::imwrite(annotatedPath.string(), annotated)) {
        throw runtime_error("Cannot write annotated image: " + annotatedPath.string());
    }
    return relative(annotatedPath, output.root);
}

} // namespace pmt
