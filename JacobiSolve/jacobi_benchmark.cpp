#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <random>
#include <omp.h>
#include "solvers.h"

using namespace std;

int main() {
    // Các kích cỡ mẫu thử nghiệm
    vector<int> sizes = {100, 200, 400, 800, 1000, 1600, 3000};
    
    // Các số lượng thread đo lường: 1 (seq), 2, 4, 8
    vector<int> threadCounts = {1, 2, 4, 8};
    int REPEATS = 3;

    cout << "Bat dau benchmark phuong phap lap Jacobi..." << endl;

    struct TestData {
        int n;
        vector<pair<int, double>> times; // {threadCount, avgTime}
    };
    vector<TestData> allTests;

    for (int n : sizes) {
        cout << "  Dang do kich thuoc ma tran n = " << n << " ..." << endl;

        // Sinh dữ liệu ma trận ngẫu nhiên TRỘI CHÉO NGHIÊM NGẶT
        mt19937 rng(42 + n);
        uniform_real_distribution<double> distVal(-10.0, 10.0);
        uniform_real_distribution<double> distDiag(1.0, 10.0);

        vector<vector<double>> A(n, vector<double>(n));
        vector<double> B(n);

        for (int i = 0; i < n; ++i) {
            double rowSum = 0.0;
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    A[i][j] = distVal(rng);
                    rowSum += abs(A[i][j]);
                }
            }
            A[i][i] = (distVal(rng) >= 0 ? 1.0 : -1.0) * (rowSum + distDiag(rng));
            B[i] = distVal(rng) * 10.0;
        }

        TestData test;
        test.n = n;

        for (int t : threadCounts) {
            // Warm-up
            if (t == 1) {
                solveJacobiSequential(A, B, 100, 1e-5);
            } else {
                solveJacobiParallel(A, B, t, 100, 1e-5);
            }

            // Đo thời gian chạy thực tế (tăng maxIterations lên 1000 cho benchmark thực sự)
            double totalTime = 0.0;
            for (int r = 0; r < REPEATS; ++r) {
                double t0 = omp_get_wtime();
                if (t == 1) {
                    solveJacobiSequential(A, B, 1000, 1e-6);
                } else {
                    solveJacobiParallel(A, B, t, 1000, 1e-6);
                }
                double t1 = omp_get_wtime();
                totalTime += (t1 - t0);
            }
            double avgTime = totalTime / REPEATS;
            cout << "    " << t << " thread(s): " << fixed << setprecision(6) << avgTime << "s" << endl;
            test.times.push_back({t, avgTime});
        }
        allTests.push_back(test);
    }

    // Ghi file data.json
    ofstream fileData("data.json");
    fileData << fixed << setprecision(9);
    fileData << "{\n  \"results\": [\n";
    for (size_t tIdx = 0; tIdx < allTests.size(); ++tIdx) {
        const auto& test = allTests[tIdx];
        fileData << "    {\n";
        fileData << "      \"n\": " << test.n << ",\n";
        fileData << "      \"times\": {\n";
        for (size_t timeIdx = 0; timeIdx < test.times.size(); ++timeIdx) {
            fileData << "        \"" << test.times[timeIdx].first << "\": " << test.times[timeIdx].second;
            if (timeIdx + 1 < test.times.size()) fileData << ",\n";
        }
        fileData << "\n      }\n    }";
        if (tIdx + 1 < allTests.size()) fileData << ",\n";
    }
    fileData << "\n  ]\n}\n";
    fileData.close();
    cout << "Da ghi file data.json!" << endl;

    cout << "Hoan thanh tat ca benchmark!" << endl;
    return 0;
}
