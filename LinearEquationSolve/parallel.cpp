// parallel.cpp - Giải hệ phương trình tuyến tính (song song - OpenMP)
// Phương pháp: Lặp Jacobi & Khử Gauss

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <omp.h>

using namespace std;
using namespace std::chrono;

// ============================================================
// Sinh ma trận chéo trội
// ============================================================
void generateDiagonallyDominantMatrix(vector<vector<double>>& A,
                                       vector<double>& b, int n, int seed = 42) {
    srand(seed);
    A.assign(n, vector<double>(n, 0.0));
    b.assign(n, 0.0);

    for (int i = 0; i < n; i++) {
        double rowSum = 0.0;
        for (int j = 0; j < n; j++) {
            if (i != j) {
                A[i][j] = (rand() % 10) - 5.0;
                rowSum += fabs(A[i][j]);
            }
        }
        A[i][i] = rowSum + (rand() % 10) + 1.0;
        b[i] = (rand() % 100) - 50.0;
    }
}

// ============================================================
// Jacobi song song (OpenMP)
// ============================================================
int jacobiParallel(const vector<vector<double>>& A,
                   const vector<double>& b,
                   vector<double>& x,
                   int n, int numThreads,
                   int maxIter = 1000, double tol = 1e-6) {
    vector<double> xNew(n, 0.0);
    x.assign(n, 0.0);
    omp_set_num_threads(numThreads);

    for (int iter = 0; iter < maxIter; iter++) {
        // Song song hóa vòng lặp tính x mới
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) {
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) sigma += A[i][j] * x[j];
            }
            xNew[i] = (b[i] - sigma) / A[i][i];
        }

        // Kiểm tra hội tụ (reduction)
        double norm = 0.0;
        #pragma omp parallel for reduction(+:norm) schedule(static)
        for (int i = 0; i < n; i++)
            norm += (xNew[i] - x[i]) * (xNew[i] - x[i]);

        // Cập nhật x song song
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) x[i] = xNew[i];

        if (sqrt(norm) < tol) return iter + 1;
    }
    return maxIter;
}

// ============================================================
// Khử Gauss song song (OpenMP)
// Song song hóa phần khử từng hàng
// ============================================================
bool gaussEliminationParallel(vector<vector<double>> A,
                               vector<double> b,
                               vector<double>& x,
                               int n, int numThreads) {
    omp_set_num_threads(numThreads);

    for (int col = 0; col < n; col++) {
        // Tìm pivot (tuần tự)
        int pivot = col;
        for (int row = col + 1; row < n; row++)
            if (fabs(A[row][col]) > fabs(A[pivot][col])) pivot = row;

        swap(A[col], A[pivot]);
        swap(b[col], b[pivot]);

        if (fabs(A[col][col]) < 1e-12) return false;

        double diagInv = 1.0 / A[col][col];

        // Song song hóa phần khử các hàng bên dưới
        #pragma omp parallel for schedule(static)
        for (int row = col + 1; row < n; row++) {
            double factor = A[row][col] * diagInv;
            for (int k = col; k < n; k++)
                A[row][k] -= factor * A[col][k];
            b[row] -= factor * b[col];
        }
    }

    // Thế ngược (tuần tự - data dependency)
    x.assign(n, 0.0);
    for (int i = n - 1; i >= 0; i--) {
        double sum = b[i];
        // Song song hóa phần tính tổng
        #pragma omp parallel for reduction(-:sum) schedule(static)
        for (int j = i + 1; j < n; j++) sum -= A[i][j] * x[j];
        x[i] = sum / A[i][i];
    }
    return true;
}

// ============================================================
// Main: benchmark với nhiều kích thước và số luồng
// ============================================================
int main() {
    vector<int> sizes       = {100, 200, 500, 1000, 2000};
    vector<int> threadCounts = {1, 2, 4, 8, 16};

    ofstream csv("results_parallel.csv");
    csv << "n,method,threads,time_ms,iterations\n";
    csv << fixed << setprecision(4);

    cout << "=== PARALLEL (OpenMP) BENCHMARK ===\n";
    cout << "Max hardware threads: " << omp_get_max_threads() << "\n\n";

    for (int n : sizes) {
        cout << "--- N = " << n << " ---\n";
        cout << left << setw(10) << "Method"
             << setw(10) << "Threads"
             << setw(16) << "Time (ms)"
             << setw(12) << "Iterations" << "\n";
        cout << string(48, '-') << "\n";

        vector<vector<double>> A;
        vector<double> b;
        generateDiagonallyDominantMatrix(A, b, n);

        for (int threads : threadCounts) {
            vector<double> x;

            // --- Jacobi Parallel ---
            auto t1 = high_resolution_clock::now();
            int iters = jacobiParallel(A, b, x, n, threads);
            auto t2 = high_resolution_clock::now();
            double tJacobi = duration<double, milli>(t2 - t1).count();

            cout << left << setw(10) << "Jacobi"
                 << setw(10) << threads
                 << setw(16) << tJacobi
                 << setw(12) << iters << "\n";
            csv << n << ",Jacobi," << threads << "," << tJacobi << "," << iters << "\n";

            // --- Gauss Parallel ---
            t1 = high_resolution_clock::now();
            gaussEliminationParallel(A, b, x, n, threads);
            t2 = high_resolution_clock::now();
            double tGauss = duration<double, milli>(t2 - t1).count();

            cout << left << setw(10) << "Gauss"
                 << setw(10) << threads
                 << setw(16) << tGauss
                 << setw(12) << "-" << "\n";
            csv << n << ",Gauss," << threads << "," << tGauss << ",1\n";
        }
        cout << "\n";
    }

    csv.close();
    cout << "Kết quả đã lưu vào: results_parallel.csv\n";
    return 0;
}
