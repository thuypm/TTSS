#include "vector_io.h"

#include <bits/stdc++.h>

using namespace std;

size_t findMatchingChar(const string& text, size_t startPos, char openChar, char closeChar) {
    int depth = 0;

    for (size_t i = startPos; i < text.size(); i++) {
        if (text[i] == openChar) {
            depth++;
        } else if (text[i] == closeChar) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }

    return string::npos;
}

vector<double> readVectorFromJson(const string& json, const string& key) {
    string quotedKey = "\"" + key + "\"";
    size_t keyPos = json.find(quotedKey);
    if (keyPos == string::npos) {
        throw runtime_error("Khong tim thay khoa: " + key);
    }

    size_t openBracket = json.find('[', keyPos);
    size_t closeBracket = json.find(']', openBracket);
    if (openBracket == string::npos || closeBracket == string::npos) {
        throw runtime_error("Du lieu cua " + key + " khong hop le.");
    }

    string valuesText = json.substr(openBracket + 1, closeBracket - openBracket - 1);
    for (char& ch : valuesText) {
        if (ch == ',') {
            ch = ' ';
        }
    }

    vector<double> values;
    stringstream ss(valuesText);
    double value;
    while (ss >> value) {
        values.push_back(value);
    }

    if (values.empty()) {
        throw runtime_error("Vector " + key + " khong duoc rong.");
    }

    return values;
}

int readIntFromJson(const string& json, const string& key) {
    string quotedKey = "\"" + key + "\"";
    size_t keyPos = json.find(quotedKey);
    if (keyPos == string::npos) {
        throw runtime_error("Khong tim thay khoa: " + key);
    }

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == string::npos) {
        throw runtime_error("Du lieu cua " + key + " khong hop le.");
    }

    stringstream ss(json.substr(colonPos + 1));
    int value;
    if (!(ss >> value)) {
        throw runtime_error("Khong doc duoc gia tri so nguyen cua " + key);
    }

    return value;
}

vector<string> readTestObjects(const string& json) {
    string testsKey = "\"tests\"";
    size_t testsPos = json.find(testsKey);
    if (testsPos == string::npos) {
        throw runtime_error("Khong tim thay mang tests.");
    }

    size_t arrayStart = json.find('[', testsPos);
    size_t arrayEnd = findMatchingChar(json, arrayStart, '[', ']');
    if (arrayStart == string::npos || arrayEnd == string::npos) {
        throw runtime_error("Mang tests khong hop le.");
    }

    vector<string> objects;
    size_t pos = arrayStart + 1;
    while (pos < arrayEnd) {
        size_t objectStart = json.find('{', pos);
        if (objectStart == string::npos || objectStart > arrayEnd) {
            break;
        }

        size_t objectEnd = findMatchingChar(json, objectStart, '{', '}');
        if (objectEnd == string::npos || objectEnd > arrayEnd) {
            throw runtime_error("Test case khong hop le.");
        }

        objects.push_back(json.substr(objectStart, objectEnd - objectStart + 1));
        pos = objectEnd + 1;
    }

    if (objects.empty()) {
        throw runtime_error("Mang tests khong duoc rong.");
    }

    return objects;
}

vector<TestCase> readTestsFromJson(const string& json) {
    vector<string> objects = readTestObjects(json);
    vector<TestCase> tests;

    for (const string& object : objects) {
        TestCase test;
        test.dimension = readIntFromJson(object, "dimension");
        test.vectorA = readVectorFromJson(object, "vectorA");
        test.vectorB = readVectorFromJson(object, "vectorB");

        if (test.dimension <= 0) {
            throw runtime_error("dimension phai lon hon 0.");
        }

        if (test.vectorA.size() != static_cast<size_t>(test.dimension) ||
            test.vectorB.size() != static_cast<size_t>(test.dimension)) {
            throw runtime_error("dimension khong khop voi vectorA hoac vectorB.");
        }

        tests.push_back(test);
    }

    return tests;
}

vector<TestCase> readTestsFromFile(const string& fileName) {
    ifstream inputFile(fileName);
    if (!inputFile) {
        throw runtime_error("Khong mo duoc file " + fileName);
    }

    stringstream buffer;
    buffer << inputFile.rdbuf();
    return readTestsFromJson(buffer.str());
}

void writeVectorJson(ofstream& output, int indent, const string& key, const vector<double>& values, bool comma) {
    output << string(indent, ' ') << "\"" << key << "\": [";
    for (size_t i = 0; i < values.size(); i++) {
        output << values[i];
        if (i + 1 < values.size()) {
            output << ", ";
        }
    }
    output << "]";
    if (comma) {
        output << ",";
    }
    output << endl;
}

void writeResultsToFile(const string& fileName, const vector<TestResult>& results) {
    ofstream outputFile(fileName);
    if (!outputFile) {
        throw runtime_error("Khong tao duoc file " + fileName);
    }

    outputFile << "{" << endl;
    outputFile << "  \"tests\": [" << endl;

    for (size_t i = 0; i < results.size(); i++) {
        const TestResult& result = results[i];

        outputFile << "    {" << endl;
        outputFile << "      \"dimension\": " << result.dimension << "," << endl;
        writeVectorJson(outputFile, 6, "result", result.result, false);
        outputFile << "    }";
        if (i + 1 < results.size()) {
            outputFile << ",";
        }
        outputFile << endl;
    }

    outputFile << "  ]" << endl;
    outputFile << "}" << endl;
}
