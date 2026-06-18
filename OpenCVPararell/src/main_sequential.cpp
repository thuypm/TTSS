#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "Config.hpp"
#include "ImageFileScanner.hpp"
#include "ImageProcessorSequential.hpp"
#include "Types.hpp"

namespace {

using namespace std;

using std::chrono::duration;
using std::chrono::steady_clock;

void printUsage() {
    cout
        << "Usage: pmt-scanner-sequential --input input --output output "
        << "--config samples/paper-config.json\n";
}

pmt::CliOptions parseArgs(int argc, char** argv) {
    pmt::CliOptions options;

    for (int i = 1; i < argc; ++i) {
        const string arg = argv[i];
        const auto requireValue = [&](const string& name) -> string {
            if (i + 1 >= argc) {
                throw runtime_error("Missing value for " + name);
            }
            return argv[++i];
        };

        if (arg == "--input") {
            options.inputDir = requireValue(arg);
        } else if (arg == "--output") {
            options.outputDir = requireValue(arg);
        } else if (arg == "--config") {
            options.configPath = requireValue(arg);
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            exit(0);
        } else {
            throw runtime_error("Unknown argument: " + arg);
        }
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parseArgs(argc, argv);
        const auto start = steady_clock::now();

        const pmt::PaperConfig config = pmt::loadPaperConfig(options.configPath);
        const pmt::OutputPaths output = pmt::prepareOutputPaths(options.outputDir);
        const auto files = pmt::scanImageFiles(options.inputDir);
        const auto results = pmt::processImageBatchSequential(files, config, output);

        const auto end = steady_clock::now();
        const double elapsedMs = duration<double, milli>(end - start).count();

        cout << "Processed " << results.size() << " image(s) in " << elapsedMs << " ms.\n";
        cout << "Mode: sequential\n";
        cout << "Threads: 1\n";
        cout << "Output: " << options.outputDir.string() << "\n";
        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        printUsage();
        return 1;
    }
}
