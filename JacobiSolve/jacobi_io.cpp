#include "jacobi_io.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdexcept>

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

string readStringFromJson(const string& json, const string& key) {
    string quotedKey = "\"" + key + "\"";
    size_t keyPos = json.find(quotedKey);
    if (keyPos == string::npos) {
        throw runtime_error("Khong tim thay khoa: " + key);
    }

    size_t colonPos = json.find(':', keyPos);
    size_t openQuote = json.find('"', colonPos + 1);
    size_t closeQuote = json.find('"', openQuote + 1);
    if (colonPos == string::npos || openQuote == string::npos || closeQuote == string::npos) {
        throw runtime_error("Du lieu cua " + key + " khong hop le.");
    }

    return json.substr(openQuote + 1, closeQuote - openQuote - 1);
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

vector<double> parseNumberArray(const string& valuesText, const string& key) {
    string normalized = valuesText;
    for (char& ch : normalized) {
        if (ch == ',') {
            ch = ' ';
        }
    }

    vector<double> values;
    stringstream ss(normalized);
    double value;
    while (ss >> value) {
        values.push_back(value);
    }

    if (values.empty()) {
        throw runtime_error("Mang " + key + " khong duoc rong.");
    }

    return values;
}

vector<double> readVectorFromJson(const string& json, const string& key) {
    string quotedKey = "\"" + key + "\"";
    size_t keyPos = json.find(quotedKey);
    if (keyPos == string::npos) {
        throw runtime_error("Khong tim thay khoa: " + key);
    }

    size_t openBracket = json.find('[', keyPos);
    size_t closeBracket = findMatchingChar(json, openBracket, '[', ']');
    if (openBracket == string::npos || closeBracket == string::npos) {
        throw runtime_error("Du lieu cua " + key + " khong hop le.");
    }

    return parseNumberArray(json.substr(openBracket + 1, closeBracket - openBracket - 1), key);
}

vector<vector<double>> readMatrixFromJson(const string& json, const string& key) {
    string quotedKey = "\"" + key + "\"";
    size_t keyPos = json.find(quotedKey);
    if (keyPos == string::npos) {
        throw runtime_error("Khong tim thay khoa: " + key);
    }

    size_t matrixStart = json.find('[', keyPos);
    size_t matrixEnd = findMatchingChar(json, matrixStart, '[', ']');
    if (matrixStart == string::npos || matrixEnd == string::npos) {
        throw runtime_error("Du lieu cua " + key + " khong hop le.");
    }

    vector<vector<double>> matrix;
    size_t pos = matrixStart + 1;
    while (pos < matrixEnd) {
        size_t rowStart = json.find('[', pos);
        if (rowStart == string::npos || rowStart > matrixEnd) {
            break;
        }

        size_t rowEnd = findMatchingChar(json, rowStart, '[', ']');
        if (rowEnd == string::npos || rowEnd > matrixEnd) {
            throw runtime_error("Hang cua " + key + " khong hop le.");
        }

        matrix.push_back(parseNumberArray(json.substr(rowStart + 1, rowEnd - rowStart - 1), key));
        pos = rowEnd + 1;
    }

    if (matrix.empty()) {
        throw runtime_error("Ma tran " + key + " khong duoc rong.");
    }

    return matrix;
}

void validateTest(const LinearSystemTest& test) {
    if (test.dimension <= 0) {
        throw runtime_error("dimension phai lon hon 0.");
    }

    if (test.matrixA.size() != static_cast<size_t>(test.dimension)) {
        throw runtime_error("So hang cua matrixA khong khop dimension.");
    }

    for (const vector<double>& row : test.matrixA) {
        if (row.size() != static_cast<size_t>(test.dimension)) {
            throw runtime_error("matrixA phai la ma tran vuong dimension x dimension.");
        }
    }

    if (test.vectorB.size() != static_cast<size_t>(test.dimension)) {
        throw runtime_error("vectorB khong khop dimension.");
    }

    if (test.expectedSolution.size() != static_cast<size_t>(test.dimension)) {
        throw runtime_error("expectedSolution khong khop dimension.");
    }
}

vector<LinearSystemTest> readTestsFromJson(const string& json) {
    vector<string> objects = readTestObjects(json);
    vector<LinearSystemTest> tests;

    for (const string& object : objects) {
        LinearSystemTest test;
        test.name = readStringFromJson(object, "name");
        test.method = readStringFromJson(object, "method");
        test.dimension = readIntFromJson(object, "dimension");
        test.matrixA = readMatrixFromJson(object, "matrixA");
        test.vectorB = readVectorFromJson(object, "vectorB");
        test.expectedSolution = readVectorFromJson(object, "expectedSolution");

        validateTest(test);
        tests.push_back(test);
    }

    return tests;
}

vector<LinearSystemTest> readTestsFromFile(const string& fileName) {
    ifstream inputFile(fileName);
    if (!inputFile) {
        throw runtime_error("Khong mo duoc file " + fileName);
    }

    stringstream buffer;
    buffer << inputFile.rdbuf();
    return readTestsFromJson(buffer.str());
}
