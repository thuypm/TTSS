#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <exception>

#include "gauss_io.h"
#include "solvers.h"

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
            cout << "\n----------------------------------------\n";
            cout << "Test Name: " << test.name << endl;
            cout << "Method: " << test.method << " | Dimension: " << test.dimension << endl;
            cout << "Expected: ";
            printVector(test.expectedSolution);
            cout << endl;

            vector<double> solution;
            try {
                if (test.method == "determinant") {
                    solution = solveCramer(test.matrixA, test.vectorB);
                } else if (test.method == "gauss") {
                    solution = solveGauss(test.matrixA, test.vectorB);
                } else if (test.method == "jacobi") {
                    solution = solveJacobi(test.matrixA, test.vectorB);
                } else {
                    cout << "Phuong phap khong hop le: " << test.method << endl;
                    continue;
                }

                cout << "Result:   ";
                printVector(solution);
                cout << endl;

                // Tinh sai so max
                double maxDiff = 0.0;
                for (size_t i = 0; i < solution.size(); ++i) {
                    maxDiff = max(maxDiff, abs(solution[i] - test.expectedSolution[i]));
                }
                cout << "Max diff: " << maxDiff;
                if (maxDiff < 1e-4) {
                    cout << " -> [PASSED]" << endl;
                } else {
                    cout << " -> [FAILED]" << endl;
                }
            } catch (const exception& error) {
                cerr << "Loi khi giai: " << error.what() << endl;
            }
        }
        cout << "\n----------------------------------------\n";
    } catch (const exception& error) {
        cerr << "Loi doc file: " << error.what() << endl;
        return 1;
    }

    return 0;
}

