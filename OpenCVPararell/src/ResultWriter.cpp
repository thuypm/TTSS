#include "ResultWriter.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pmt {

using namespace std;

using std::filesystem::create_directories;
using std::filesystem::path;

namespace {

string escapeJson(const string& value) {
    ostringstream out;
    for (const char ch : value) {
        switch (ch) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                out << ch;
                break;
        }
    }
    return out.str();
}

string jsonStringOrNull(const string& value) {
    if (value.empty()) {
        return "null";
    }
    return "\"" + escapeJson(value) + "\"";
}

string jsonArray2(const vector<vector<string>>& values) {
    ostringstream out;
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "[";
        for (size_t j = 0; j < values[i].size(); ++j) {
            if (j > 0) {
                out << ",";
            }
            out << "\"" << escapeJson(values[i][j]) << "\"";
        }
        out << "]";
    }
    out << "]";
    return out.str();
}

string jsonArray3(const vector<vector<vector<string>>>& values) {
    ostringstream out;
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << jsonArray2(values[i]);
    }
    out << "]";
    return out.str();
}

string csvEscape(const string& value) {
    if (value.find_first_of("\",\n\r") == string::npos) {
        return value;
    }

    string escaped = "\"";
    for (const char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped += ch;
        }
    }
    escaped += "\"";
    return escaped;
}

void ensureWritable(const ofstream& file, const path& filePath) {
    if (!file) {
        throw runtime_error("Cannot write file: " + filePath.string());
    }
}

} // namespace

OutputPaths prepareOutputPaths(const path& outputDir) {
    OutputPaths output;
    output.root = outputDir;
    output.warpedDir = outputDir / "warped";
    output.annotatedDir = outputDir / "annotated";
    output.logsDir = outputDir / "logs";

    create_directories(output.warpedDir);
    create_directories(output.annotatedDir);
    create_directories(output.logsDir);

    return output;
}

void writeResultsJson(
    const path& outputDir,
    const path& inputDir,
    const vector<ImageProcessResult>& results,
    double elapsedMs
) {
    const auto filePath = outputDir / "results.json";
    ofstream file(filePath);
    ensureWritable(file, filePath);

    const auto successCount = count_if(results.begin(), results.end(), [](const ImageProcessResult& result) {
        return result.success;
    });

    file << fixed << setprecision(3);
    file << "{\n";
    file << "  \"inputDir\": \"" << escapeJson(inputDir.string()) << "\",\n";
    file << "  \"total\": " << results.size() << ",\n";
    file << "  \"success\": " << successCount << ",\n";
    file << "  \"failed\": " << (results.size() - successCount) << ",\n";
    file << "  \"elapsedMs\": " << elapsedMs << ",\n";
    file << "  \"items\": [\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        file << "    {\n";
        file << "      \"sourcePath\": \"" << escapeJson(result.sourcePath) << "\",\n";
        file << "      \"fileName\": \"" << escapeJson(result.fileName) << "\",\n";
        file << "      \"pageNum\": " << result.pageNum << ",\n";
        file << "      \"success\": " << (result.success ? "true" : "false") << ",\n";
        file << "      \"fallback\": " << (result.data.fallback ? "true" : "false") << ",\n";
        file << "      \"studentId\": " << jsonArray2(result.data.studentId) << ",\n";
        file << "      \"key\": " << jsonArray2(result.data.key) << ",\n";
        file << "      \"partOne\": " << jsonArray2(result.data.partOne) << ",\n";
        file << "      \"partTwo\": " << jsonArray2(result.data.partTwo) << ",\n";
        file << "      \"partThree\": " << jsonArray3(result.data.partThree) << ",\n";
        file << "      \"warpedImage\": " << jsonStringOrNull(result.warpedImagePath) << ",\n";
        file << "      \"annotatedImage\": " << jsonStringOrNull(result.annotatedImagePath) << ",\n";
        file << "      \"error\": " << jsonStringOrNull(result.error) << ",\n";
        file << "      \"elapsedMs\": " << result.elapsedMs << "\n";
        file << "    }" << (i + 1 == results.size() ? "\n" : ",\n");
    }

    file << "  ]\n";
    file << "}\n";
}

void writeResultsCsv(
    const path& outputDir,
    const vector<ImageProcessResult>& results
) {
    const auto filePath = outputDir / "results.csv";
    ofstream file(filePath);
    ensureWritable(file, filePath);

    file << "fileName,success,fallback,studentId,key,scoreLikeData,error,elapsedMs,warpedImage,annotatedImage\n";

    for (const auto& result : results) {
        ostringstream scoreLikeData;
        scoreLikeData << "{"
                      << "\"partOne\":" << jsonArray2(result.data.partOne) << ","
                      << "\"partTwo\":" << jsonArray2(result.data.partTwo) << ","
                      << "\"partThree\":" << jsonArray3(result.data.partThree) << "}";

        file << csvEscape(result.fileName) << ","
             << (result.success ? "true" : "false") << ","
             << (result.data.fallback ? "true" : "false") << ","
             << csvEscape(jsonArray2(result.data.studentId)) << ","
             << csvEscape(jsonArray2(result.data.key)) << ","
             << csvEscape(scoreLikeData.str()) << ","
             << csvEscape(result.error) << ","
             << result.elapsedMs << ","
             << csvEscape(result.warpedImagePath) << ","
             << csvEscape(result.annotatedImagePath) << "\n";
    }
}

void writeErrorLog(
    const path& outputDir,
    const vector<ImageProcessResult>& results
) {
    const auto filePath = outputDir / "logs" / "errors.json";
    ofstream file(filePath);
    ensureWritable(file, filePath);

    file << "[\n";
    bool first = true;
    for (const auto& result : results) {
        if (result.success) {
            continue;
        }

        if (!first) {
            file << ",\n";
        }
        first = false;

        file << "  {\n";
        file << "    \"sourcePath\": \"" << escapeJson(result.sourcePath) << "\",\n";
        file << "    \"fileName\": \"" << escapeJson(result.fileName) << "\",\n";
        file << "    \"pageNum\": " << result.pageNum << ",\n";
        file << "    \"error\": " << jsonStringOrNull(result.error) << "\n";
        file << "  }";
    }
    file << "\n]\n";
}

} // namespace pmt
