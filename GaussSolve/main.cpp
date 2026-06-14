#include <bits/stdc++.h>

#include "gauss_io.h"

using namespace std;

void printVector(const vector<double>& values) {
    cout << "[";
    for (size_t i = 0; i < values.size(); i++) {
        cout << values[i];
        if (i + 1 < values.size()) {
            cout << ", ";
        }
    }
    cout << "]";
}

int main() {
    const string inputFileName = "input.json";

    try {
        vector<LinearSystemTest> tests = readTestsFromFile(inputFileName);

        cout << "Da doc " << tests.size() << " bo test tu " << inputFileName << endl;
        for (const LinearSystemTest& test : tests) {
            cout << "- " << test.name
                 << " | method: " << test.method
                 << " | dimension: " << test.dimension
                 << " | expectedSolution: ";
            printVector(test.expectedSolution);
            cout << endl;

            // @todo: Them thuat toan giai he phuong trinh tuyen tinh o day.
            // Co the cai dat tinh dinh thuc/Cramer, khu Gauss hoac Jacobi theo test.method.
        }
    } catch (const exception& error) {
        cerr << "Loi: " << error.what() << endl;
        return 1;
    }

    return 0;
}
