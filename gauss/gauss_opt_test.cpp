#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <random>
#include <omp.h>

using namespace std;

// 1. Phép giải Gauss gốc (2D vector, parallel for lặp lại)
vector<double> solveGaussOriginal(const vector<vector<double>>& A, const vector<double>& B, int numThreads) {
    int n = A.size();
    vector<vector<double>> tempA = A;
    vector<double> tempB = B;

    omp_set_num_threads(numThreads);

    for (int i = 0; i < n; ++i) {
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

// 2. Phép giải Gauss tối ưu (Mảng phẳng 1D + Một vùng song song duy nhất)
vector<double> solveGaussOptimized(const vector<double>& A_flat, const vector<double>& B, int n, int numThreads) {
    vector<double> tempA = A_flat;
    vector<double> tempB = B;

    omp_set_num_threads(numThreads);

    #pragma omp parallel
    {
        for (int i = 0; i < n; ++i) {
            // 1. Chọn phần tử trội tuần tự trên luồng Single
            #pragma omp single
            {
                int pivot = i;
                for (int j = i + 1; j < n; ++j) {
                    if (abs(tempA[j * n + i]) > abs(tempA[pivot * n + i])) {
                        pivot = j;
                    }
                }
                if (pivot != i) {
                    for (int k = 0; k < n; ++k) {
                        swap(tempA[i * n + k], tempA[pivot * n + k]);
                    }
                    swap(tempB[i], tempB[pivot]);
                }
            } // Rào chắn ngầm định tại đây giúp đồng bộ hóa tất cả các luồng trước khi bắt đầu khử

            // 2. Khử hàng song song sử dụng omp for (không khởi tạo lại vùng song song)
            #pragma omp for
            for (int j = i + 1; j < n; ++j) {
                double factor = tempA[j * n + i] / tempA[i * n + i];
                tempA[j * n + i] = 0.0;
                for (int k = i + 1; k < n; ++k) {
                    tempA[j * n + k] -= factor * tempA[i * n + k];
                }
                tempB[j] -= factor * tempB[i];
            } // Rào chắn ngầm định đồng bộ các luồng cho bước tiếp theo
        }
    }

    // 3. Thế ngược (Back-substitution) phẳng
    vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        #pragma omp parallel for reduction(+:sum) if(n - i - 1 >= 16)
        for (int j = i + 1; j < n; ++j) {
            sum += tempA[i * n + j] * x[j];
        }
        x[i] = (tempB[i] - sum) / tempA[i * n + i];
    }
    return x;
}

int main() {
    int n = 5000;
    vector<int> threadCounts = {1, 2, 8, 12};
    int REPEATS = 2; // Lặp 2 lần để lấy trung bình nhanh hơn cho N=5000

    cout << "Kich thuoc thu nghiem so sanh N = " << n << endl;

    // Sinh ma trận ngẫu nhiên trội chéo
    mt19937 rng(42);
    uniform_real_distribution<double> distVal(-10.0, 10.0);
    uniform_real_distribution<double> distDiag(5.0, 20.0);

    vector<vector<double>> A(n, vector<double>(n));
    vector<double> A_flat(n * n);
    vector<double> B(n);

    for (int i = 0; i < n; ++i) {
        double rowSum = 0.0;
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                double val = distVal(rng);
                A[i][j] = val;
                A_flat[i * n + j] = val;
                rowSum += abs(val);
            }
        }
        double diag = (distVal(rng) >= 0 ? 1.0 : -1.0) * (rowSum + distDiag(rng));
        A[i][i] = diag;
        A_flat[i * n + i] = diag;
        B[i] = distVal(rng) * 10.0;
    }

    // Kiểm tra tính đúng đắn trước
    cout << "Kiem tra tinh dung dan..." << endl;
    vector<double> x_orig = solveGaussOriginal(A, B, 8);
    vector<double> x_opt = solveGaussOptimized(A_flat, B, n, 8);

    double maxDiff = 0.0;
    for (int i = 0; i < n; ++i) {
        maxDiff = max(maxDiff, abs(x_orig[i] - x_opt[i]));
    }
    cout << "Max difference: " << maxDiff << (maxDiff < 1e-9 ? " (PASSED)" : " (FAILED)") << endl;

    // Đo thời gian bản gốc
    cout << "Bat dau do ban Goc (Original)..." << endl;
    vector<double> timesOrig;
    for (int t : threadCounts) {
        // Warmup
        solveGaussOriginal(A, B, t);

        double totalTime = 0.0;
        for (int r = 0; r < REPEATS; ++r) {
            double t0 = omp_get_wtime();
            solveGaussOriginal(A, B, t);
            double t1 = omp_get_wtime();
            totalTime += (t1 - t0);
        }
        double avgTime = totalTime / REPEATS;
        cout << "  " << t << " thread(s): " << avgTime << "s" << endl;
        timesOrig.push_back(avgTime);
    }

    // Đo thời gian bản tối ưu
    cout << "Bat dau do ban Toi uu (Optimized)..." << endl;
    vector<double> timesOpt;
    for (int t : threadCounts) {
        // Warmup
        solveGaussOptimized(A_flat, B, n, t);

        double totalTime = 0.0;
        for (int r = 0; r < REPEATS; ++r) {
            double t0 = omp_get_wtime();
            solveGaussOptimized(A_flat, B, n, t);
            double t1 = omp_get_wtime();
            totalTime += (t1 - t0);
        }
        double avgTime = totalTime / REPEATS;
        cout << "  " << t << " thread(s): " << avgTime << "s" << endl;
        timesOpt.push_back(avgTime);
    }

    // Ghi file so sanh json
    ofstream file("opt_data.json");
    file << fixed << setprecision(6);
    file << "{\n";
    file << "  \"n\": " << n << ",\n";
    file << "  \"threads\": [1, 2, 8, 12],\n";
    file << "  \"original\": [";
    for (size_t i = 0; i < timesOrig.size(); ++i) {
        file << timesOrig[i];
        if (i + 1 < timesOrig.size()) file << ", ";
    }
    file << "],\n";
    file << "  \"optimized\": [";
    for (size_t i = 0; i < timesOpt.size(); ++i) {
        file << timesOpt[i];
        if (i + 1 < timesOpt.size()) file << ", ";
    }
    file << "]\n";
    file << "}\n";
    file.close();
    cout << "Da ghi file opt_data.json!" << endl;

    return 0;
}
