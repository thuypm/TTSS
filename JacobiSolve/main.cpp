#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <exception>

#include "jacobi_io.h"
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

            vector<double> solutionSeq;
            vector<double> solutionPar;
            
            try {
                if (test.method == "jacobi") {
                    cout << "Running Jacobi Sequential..." << endl;
                    solutionSeq = solveJacobiSequential(test.matrixA, test.vectorB);
                    cout << "Seq Result:   ";
                    printVector(solutionSeq);
                    cout << endl;
                    
                    cout << "Running Jacobi Parallel..." << endl;
                    solutionPar = solveJacobiParallel(test.matrixA, test.vectorB, 2);
                    cout << "Par Result:   ";
                    printVector(solutionPar);
                    cout << endl;
                    
                    // Kiem tra sai so cua Seq
                    double maxDiff = 0.0;
                    for (size_t i = 0; i < solutionSeq.size(); ++i) {
                        maxDiff = max(maxDiff, abs(solutionSeq[i] - test.expectedSolution[i]));
                    }
                    cout << "Max diff (Seq vs Expected): " << maxDiff;
                    if (maxDiff < 1e-4) cout << " -> [PASSED]" << endl;
                    else cout << " -> [FAILED]" << endl;
                    
                } else {
                    cout << "Phuong phap " << test.method << " khong nam trong muc tieu test chinh cua chuong trinh nay." << endl;
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
