#include "Config.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pmt {

using namespace std;

using std::filesystem::exists;
using std::filesystem::path;

namespace {

struct JsonValue {
    enum class Type {
        Null,
        Number,
        String,
        Object,
        Array
    };

    Type type = Type::Null;
    double number = 0.0;
    string text;
    map<string, JsonValue> object;
    vector<JsonValue> array;
};

class JsonParser {
public:
    explicit JsonParser(string source) : source_(move(source)) {}

    JsonValue parse() {
        JsonValue value = parseValue();
        skipWhitespace();
        if (position_ != source_.size()) {
            throw runtime_error("Unexpected content after JSON root.");
        }
        return value;
    }

private:
    string source_;
    size_t position_ = 0;

    void skipWhitespace() {
        while (position_ < source_.size() && isspace(static_cast<unsigned char>(source_[position_]))) {
            ++position_;
        }
    }

    char peek() {
        skipWhitespace();
        if (position_ >= source_.size()) {
            throw runtime_error("Unexpected end of JSON.");
        }
        return source_[position_];
    }

    char consume() {
        if (position_ >= source_.size()) {
            throw runtime_error("Unexpected end of JSON.");
        }
        return source_[position_++];
    }

    void expect(char expected) {
        skipWhitespace();
        const char actual = consume();
        if (actual != expected) {
            throw runtime_error(string("Expected '") + expected + "' but found '" + actual + "'.");
        }
    }

    JsonValue parseValue() {
        skipWhitespace();
        const char ch = peek();
        if (ch == '{') {
            return parseObject();
        }
        if (ch == '[') {
            return parseArray();
        }
        if (ch == '"') {
            JsonValue value;
            value.type = JsonValue::Type::String;
            value.text = parseString();
            return value;
        }
        if (ch == '-' || isdigit(static_cast<unsigned char>(ch))) {
            return parseNumber();
        }
        if (source_.compare(position_, 4, "null") == 0) {
            position_ += 4;
            return {};
        }

        throw runtime_error("Unsupported JSON value.");
    }

    JsonValue parseObject() {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        expect('{');
        skipWhitespace();
        if (peek() == '}') {
            consume();
            return value;
        }

        while (true) {
            const string key = parseString();
            expect(':');
            value.object.emplace(key, parseValue());

            skipWhitespace();
            const char separator = consume();
            if (separator == '}') {
                break;
            }
            if (separator != ',') {
                throw runtime_error("Expected ',' or '}' in JSON object.");
            }
        }

        return value;
    }

    JsonValue parseArray() {
        JsonValue value;
        value.type = JsonValue::Type::Array;
        expect('[');
        skipWhitespace();
        if (peek() == ']') {
            consume();
            return value;
        }

        while (true) {
            value.array.push_back(parseValue());

            skipWhitespace();
            const char separator = consume();
            if (separator == ']') {
                break;
            }
            if (separator != ',') {
                throw runtime_error("Expected ',' or ']' in JSON array.");
            }
        }

        return value;
    }

    string parseString() {
        expect('"');
        string result;
        while (position_ < source_.size()) {
            const char ch = consume();
            if (ch == '"') {
                return result;
            }
            if (ch != '\\') {
                result.push_back(ch);
                continue;
            }

            const char escaped = consume();
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                default:
                    throw runtime_error("Unsupported JSON string escape.");
            }
        }

        throw runtime_error("Unterminated JSON string.");
    }

    JsonValue parseNumber() {
        const size_t start = position_;
        if (source_[position_] == '-') {
            ++position_;
        }
        while (position_ < source_.size() && isdigit(static_cast<unsigned char>(source_[position_]))) {
            ++position_;
        }
        if (position_ < source_.size() && source_[position_] == '.') {
            ++position_;
            while (position_ < source_.size() && isdigit(static_cast<unsigned char>(source_[position_]))) {
                ++position_;
            }
        }

        JsonValue value;
        value.type = JsonValue::Type::Number;
        value.number = stod(source_.substr(start, position_ - start));
        return value;
    }
};

const JsonValue& requireField(const JsonValue& value, const string& key) {
    if (value.type != JsonValue::Type::Object) {
        throw runtime_error("Expected JSON object while reading " + key + ".");
    }

    const auto it = value.object.find(key);
    if (it == value.object.end()) {
        throw runtime_error("Missing config field: " + key);
    }
    return it->second;
}

int readInt(const JsonValue& value, const string& fieldName) {
    if (value.type != JsonValue::Type::Number) {
        throw runtime_error("Expected number for config field: " + fieldName);
    }
    return static_cast<int>(value.number);
}

cv::Point readPointArray(const JsonValue& value, const string& fieldName) {
    if (value.type != JsonValue::Type::Array || value.array.size() != 2) {
        throw runtime_error("Expected [x, y] for config field: " + fieldName);
    }

    return {
        readInt(value.array[0], fieldName + "[0]"),
        readInt(value.array[1], fieldName + "[1]")
    };
}

cv::Point readPointObject(const JsonValue& value, const string& fieldName) {
    return {
        readInt(requireField(value, "x"), fieldName + ".x"),
        readInt(requireField(value, "y"), fieldName + ".y")
    };
}

BubbleBox readBubbleBox(const JsonValue& value, const string& fieldName) {
    BubbleBox box;
    box.topLeft = readPointArray(requireField(value, "topLeft"), fieldName + ".topLeft");
    box.bottomRight = readPointArray(requireField(value, "bottomRight"), fieldName + ".bottomRight");
    return box;
}

map<string, BubbleBox> readBubbleGroup(const JsonValue& value, const string& fieldName) {
    if (value.type != JsonValue::Type::Object) {
        throw runtime_error("Expected bubble group object for config field: " + fieldName);
    }

    map<string, BubbleBox> group;
    for (const auto& [label, boxValue] : value.object) {
        group.emplace(label, readBubbleBox(boxValue, fieldName + "." + label));
    }

    if (group.empty()) {
        throw runtime_error("Bubble group cannot be empty: " + fieldName);
    }

    return group;
}

vector<map<string, BubbleBox>> readBubbleGroupList(const JsonValue& value, const string& fieldName) {
    if (value.type != JsonValue::Type::Array) {
        throw runtime_error("Expected bubble group array for config field: " + fieldName);
    }

    vector<map<string, BubbleBox>> groups;
    groups.reserve(value.array.size());
    for (size_t i = 0; i < value.array.size(); ++i) {
        groups.push_back(readBubbleGroup(value.array[i], fieldName + "[" + to_string(i) + "]"));
    }
    return groups;
}

vector<vector<map<string, BubbleBox>>> readNestedBubbleGroupList(const JsonValue& value, const string& fieldName) {
    if (value.type != JsonValue::Type::Array) {
        throw runtime_error("Expected nested bubble group array for config field: " + fieldName);
    }

    vector<vector<map<string, BubbleBox>>> groups;
    groups.reserve(value.array.size());
    for (size_t i = 0; i < value.array.size(); ++i) {
        groups.push_back(readBubbleGroupList(value.array[i], fieldName + "[" + to_string(i) + "]"));
    }
    return groups;
}

void validateConfig(const PaperConfig& config) {
    if (config.width <= 0 || config.height <= 0) {
        throw runtime_error("Config size.width and size.height must be greater than 0.");
    }
    if (config.studentId.empty()) {
        throw runtime_error("Config studentId cannot be empty.");
    }
    if (config.key.empty()) {
        throw runtime_error("Config key cannot be empty.");
    }
    if (config.partOne.empty()) {
        throw runtime_error("Config partOne cannot be empty.");
    }
    if (config.partTwo.empty()) {
        throw runtime_error("Config partTwo cannot be empty.");
    }
    if (config.partThree.empty()) {
        throw runtime_error("Config partThree cannot be empty.");
    }
}

} // namespace

PaperConfig loadPaperConfig(const path& configPath) {
    if (!exists(configPath)) {
        throw runtime_error("Config file not found: " + configPath.string());
    }

    ifstream file(configPath);
    if (!file) {
        throw runtime_error("Cannot open config file: " + configPath.string());
    }

    stringstream buffer;
    buffer << file.rdbuf();
    const JsonValue root = JsonParser(buffer.str()).parse();

    PaperConfig config;
    const JsonValue& size = requireField(root, "size");
    config.width = readInt(requireField(size, "width"), "size.width");
    config.height = readInt(requireField(size, "height"), "size.height");
    config.score = readPointObject(requireField(root, "score"), "score");
    config.note = readPointObject(requireField(root, "note"), "note");
    config.studentId = readBubbleGroupList(requireField(root, "studentId"), "studentId");
    config.key = readBubbleGroupList(requireField(root, "key"), "key");
    config.partOne = readBubbleGroupList(requireField(root, "partOne"), "partOne");
    config.partTwo = readBubbleGroupList(requireField(root, "partTwo"), "partTwo");
    config.partThree = readNestedBubbleGroupList(requireField(root, "partThree"), "partThree");
    validateConfig(config);
    return config;
}

Backend parseBackend(const string& value) {
    string normalized = value;
    transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });

    if (normalized == "cpu") {
        return Backend::Cpu;
    }
    if (normalized == "cuda") {
        return Backend::Cuda;
    }

    throw runtime_error("Unsupported backend: " + value);
}

} // namespace pmt
