#include <bits/stdc++.h>
#include <omp.h>

#include "vector_io.h"

using namespace std;

vector<double> sumVectors(const vector<double>& a, const vector<double>& b) {
    // Lay so chieu cua vector.
    int n = static_cast<int>(a.size());
    // Tao vector ket qua co cung kich thuoc.
    vector<double> result(n);

    // Tinh tong song song: moi luong xu ly mot phan phan tu.
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        result[i] = a[i] + b[i];
    }

    return result;
}

int main() {
    // Khai bao ten file dau vao va file ket qua.
    const string inputFileName = "input.json";
    const string outputFileName = "output.json";

    try {
        // Doc danh sach cac bo test tu file JSON.
        vector<TestCase> tests = readTestsFromFile(inputFileName);
        vector<TestResult> results;
        results.reserve(tests.size());

        // Tinh tong tung cap vector trong moi bo test.
        for (const TestCase& test : tests) {
            results.push_back({test.dimension, sumVectors(test.vectorA, test.vectorB)});
        }

        // Ghi ket qua tinh duoc ra file output.json.
        writeResultsToFile(outputFileName, results);
        cout << "Da chay " << tests.size() << " bo test va ghi ket qua ra " << outputFileName << endl;
    } catch (const exception& error) {
        // Thong bao loi neu doc file, xu ly du lieu hoac ghi file that bai.
        cerr << "Loi: " << error.what() << endl;
        return 1;
    }

    return 0;
}
