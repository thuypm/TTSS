#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <random>
#include <omp.h>

using namespace std;

// Giải hệ phương trình bằng khử Gauss song song bằng OpenMP
vector<double> solveGauss(const vector<vector<double>>& A, const vector<double>& B, int numThreads) {
    int n = A.size();
    vector<vector<double>> tempA = A;
    vector<double> tempB = B;

    omp_set_num_threads(numThreads);

    for (int i = 0; i < n; ++i) {
        // Chọn phần tử trội (partial pivoting)
        int pivot = i;
        for (int j = i + 1; j < n; ++j) {
            if (abs(tempA[j][i]) > abs(tempA[pivot][i])) {
                pivot = j;
            }
        }
        if (pivot != i) {
            swap(tempA[i], tempA[pivot]);
            swap(tempB[i], tempB[pivot]);
        }

        // Khử xuôi song song hóa bằng OpenMP
        #pragma omp parallel for if(n - i >= 16)
        for (int j = i + 1; j < n; ++j) {
            double factor = tempA[j][i] / tempA[i][i];
            tempA[j][i] = 0.0;
            for (int k = i + 1; k < n; ++k) {
                tempA[j][k] -= factor * tempA[i][k];
            }
            tempB[j] -= factor * tempB[i];
        }
    }

    // Thế ngược song song hóa bằng OpenMP reduction
    vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        #pragma omp parallel for reduction(+:sum) if(n - i - 1 >= 16)
        for (int j = i + 1; j < n; ++j) {
            sum += tempA[i][j] * x[j];
        }
        x[i] = (tempB[i] - sum) / tempA[i][i];
    }
    return x;
}

int main() {
    // Các kích cỡ mẫu thử nghiệm
    vector<int> sizes = {100, 200, 400, 800, 1000, 1600, 3000, 5000};
    
    // Các số lượng thread đo lường: 1 (seq), 2, 8, và 12 (max máy hiện tại)
    vector<int> threadCounts = {1, 2, 8, 12};
    int REPEATS = 5;

    cout << "Bat dau benchmark phuong phap khu Gauss song song..." << endl;

    // Cấu trúc dữ liệu để lưu các bộ test phục vụ việc ghi file json sau khi chạy
    struct TestData {
        int n;
        vector<vector<double>> A;
        vector<double> B;
        vector<double> x;
        vector<pair<int, double>> times; // {threadCount, avgTime}
    };
    vector<TestData> allTests;

    for (int n : sizes) {
        cout << "  Dang do kich thuoc ma tran n = " << n << " ..." << endl;

        // Sinh dữ liệu ma trận ngẫu nhiên trội chéo để đảm bảo tính giải được
        mt19937 rng(42 + n);
        uniform_real_distribution<double> distVal(-10.0, 10.0);
        uniform_real_distribution<double> distDiag(5.0, 20.0);

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
            // Giá trị đường chéo chính lớn hơn tổng các phần tử khác trên hàng
            A[i][i] = (distVal(rng) >= 0 ? 1.0 : -1.0) * (rowSum + distDiag(rng));
            B[i] = distVal(rng) * 10.0;
        }

        TestData test;
        test.n = n;
        test.A = A;
        test.B = B;

        // Giải nghiệm tham chiếu (chạy 1 thread) để lưu kết quả nghiệm
        test.x = solveGauss(A, B, 1);

        for (int t : threadCounts) {
            // Warm-up
            solveGauss(A, B, t);

            // Đo thời gian chạy thực tế
            double totalTime = 0.0;
            for (int r = 0; r < REPEATS; ++r) {
                double t0 = omp_get_wtime();
                solveGauss(A, B, t);
                double t1 = omp_get_wtime();
                totalTime += (t1 - t0);
            }
            double avgTime = totalTime / REPEATS;
            cout << "    " << t << " thread(s): " << fixed << setprecision(6) << avgTime << "s" << endl;
            test.times.push_back({t, avgTime});
        }
        allTests.push_back(test);

        // Ghi luôn file riêng cho kích thước >= 1000 để tránh tải nặng RAM
        if (n >= 1000) {
            // Ghi input_N.json
            string inName = "input_" + to_string(n) + ".json";
            ofstream fileIn(inName);
            fileIn << fixed << setprecision(6);
            fileIn << "{\n";
            fileIn << "  \"dimension\": " << n << ",\n";
            fileIn << "  \"matrixA\": [\n";
            for (int i = 0; i < n; ++i) {
                fileIn << "    [";
                for (int j = 0; j < n; ++j) {
                    fileIn << A[i][j];
                    if (j + 1 < n) fileIn << ", ";
                }
                fileIn << "]";
                if (i + 1 < n) fileIn << ",\n";
            }
            fileIn << "\n  ],\n";
            fileIn << "  \"vectorB\": [";
            for (int i = 0; i < n; ++i) {
                fileIn << B[i];
                if (i + 1 < n) fileIn << ", ";
            }
            fileIn << "]\n";
            fileIn << "}\n";
            fileIn.close();
            cout << "  -> Da ghi file " << inName << endl;

            // Ghi output_N.json
            string outName = "output_" + to_string(n) + ".json";
            ofstream fileOut(outName);
            fileOut << fixed << setprecision(9);
            fileOut << "{\n";
            fileOut << "  \"dimension\": " << n << ",\n";
            fileOut << "  \"solution\": [";
            for (int i = 0; i < n; ++i) {
                fileOut << test.x[i];
                if (i + 1 < n) fileOut << ", ";
            }
            fileOut << "]\n";
            fileOut << "}\n";
            fileOut.close();
            cout << "  -> Da ghi file " << outName << endl;
        }
    }

    // 1. Ghi file input.json (chỉ chứa các test case <= 800)
    ofstream fileIn("input.json");
    fileIn << fixed << setprecision(6);
    fileIn << "{\n  \"tests\": [\n";
    bool firstIn = true;
    for (size_t tIdx = 0; tIdx < allTests.size(); ++tIdx) {
        const auto& test = allTests[tIdx];
        if (test.n >= 1000) continue; // Bỏ qua các case lớn
        if (!firstIn) fileIn << ",\n";
        firstIn = false;
        fileIn << "    {\n";
        fileIn << "      \"dimension\": " << test.n << ",\n";
        fileIn << "      \"matrixA\": [\n";
        for (int i = 0; i < test.n; ++i) {
            fileIn << "        [";
            for (int j = 0; j < test.n; ++j) {
                fileIn << test.A[i][j];
                if (j + 1 < test.n) fileIn << ", ";
            }
            fileIn << "]";
            if (i + 1 < test.n) fileIn << ",\n";
        }
        fileIn << "\n      ],\n";
        fileIn << "      \"vectorB\": [";
        for (int i = 0; i < test.n; ++i) {
            fileIn << test.B[i];
            if (i + 1 < test.n) fileIn << ", ";
        }
        fileIn << "]\n";
        fileIn << "    }";
    }
    fileIn << "\n  ]\n}\n";
    fileIn.close();
    cout << "Da ghi file input.json (n <= 800)!" << endl;

    // 2. Ghi file output.json (chỉ chứa các test case <= 800)
    ofstream fileOut("output.json");
    fileOut << fixed << setprecision(9);
    fileOut << "{\n  \"results\": [\n";
    bool firstOut = true;
    for (size_t tIdx = 0; tIdx < allTests.size(); ++tIdx) {
        const auto& test = allTests[tIdx];
        if (test.n >= 1000) continue; // Bỏ qua các case lớn
        if (!firstOut) fileOut << ",\n";
        firstOut = false;
        fileOut << "    {\n";
        fileOut << "      \"dimension\": " << test.n << ",\n";
        fileOut << "      \"solution\": [";
        for (int i = 0; i < test.n; ++i) {
            fileOut << test.x[i];
            if (i + 1 < test.n) fileOut << ", ";
        }
        fileOut << "]\n";
        fileOut << "    }";
    }
    fileOut << "\n  ]\n}\n";
    fileOut.close();
    cout << "Da ghi file output.json (n <= 800)!" << endl;

    // 3. Ghi file data.json (chứa tất cả thời gian của cả case nhỏ và case lớn)
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
    cout << "Da ghi file data.json (tat ca cac case)!" << endl;

    cout << "Hoan thanh tat ca benchmark!" << endl;
    return 0;
}
