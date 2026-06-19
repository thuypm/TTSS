// sequential.cpp - Giải hệ phương trình tuyến tính (tuần tự)
// Phương pháp: Lặp Jacobi & Khử Gauss

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cstdlib>

using namespace std;
using namespace std::chrono;

// ============================================================
// Sinh ma trận chéo trội (đảm bảo Jacobi hội tụ)
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
                A[i][j] = (rand() % 10) - 5.0;  // [-5, 5)
                rowSum += fabs(A[i][j]);
            }
        }
        A[i][i] = rowSum + (rand() % 10) + 1.0; // đảm bảo chéo trội
        b[i] = (rand() % 100) - 50.0;
    }
}

// ============================================================
// Jacobi tuần tự
// ============================================================
int jacobiSequential(const vector<vector<double>>& A,
                     const vector<double>& b,
                     vector<double>& x,
                     int n, int maxIter = 1000, double tol = 1e-6) {
    vector<double> xNew(n, 0.0);
    x.assign(n, 0.0);

    for (int iter = 0; iter < maxIter; iter++) {
        for (int i = 0; i < n; i++) {
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) sigma += A[i][j] * x[j];
            }
            xNew[i] = (b[i] - sigma) / A[i][i];
        }

        // Kiểm tra hội tụ
        double norm = 0.0;
        for (int i = 0; i < n; i++)
            norm += (xNew[i] - x[i]) * (xNew[i] - x[i]);
        x = xNew;
        if (sqrt(norm) < tol) return iter + 1;
    }
    return maxIter;
}

// ============================================================
// Khử Gauss tuần tự
// ============================================================
bool gaussEliminationSequential(vector<vector<double>> A,
                                  vector<double> b,
                                  vector<double>& x, int n) {
    // Khử Gauss có chọn pivot
    for (int col = 0; col < n; col++) {
        int pivot = col;
        for (int row = col + 1; row < n; row++)
            if (fabs(A[row][col]) > fabs(A[pivot][col])) pivot = row;

        swap(A[col], A[pivot]);
        swap(b[col], b[pivot]);

        if (fabs(A[col][col]) < 1e-12) return false;

        for (int row = col + 1; row < n; row++) {
            double factor = A[row][col] / A[col][col];
            for (int k = col; k < n; k++)
                A[row][k] -= factor * A[col][k];
            b[row] -= factor * b[col];
        }
    }

    // Thế ngược
    x.assign(n, 0.0);
    for (int i = n - 1; i >= 0; i--) {
        double sum = b[i];
        for (int j = i + 1; j < n; j++) sum -= A[i][j] * x[j];
        x[i] = sum / A[i][i];
    }
    return true;
}

// ============================================================
// Main: chạy benchmark và ghi kết quả ra CSV
// ============================================================
int main() {
    vector<int> sizes = {100, 200, 500, 1000, 2000};
    ofstream csv("results_sequential.csv");
    csv << "n,method,time_ms,iterations\n";
    csv << fixed << setprecision(4);

    cout << "=== SEQUENTIAL BENCHMARK ===\n\n";
    cout << left << setw(8) << "N"
         << setw(12) << "Method"
         << setw(16) << "Time (ms)"
         << setw(12) << "Iterations" << "\n";
    cout << string(48, '-') << "\n";

    for (int n : sizes) {
        vector<vector<double>> A;
        vector<double> b, x;
        generateDiagonallyDominantMatrix(A, b, n);

        // --- Jacobi ---
        auto t1 = high_resolution_clock::now();
        int iters = jacobiSequential(A, b, x, n);
        auto t2 = high_resolution_clock::now();
        double tJacobi = duration<double, milli>(t2 - t1).count();

        cout << left << setw(8) << n
             << setw(12) << "Jacobi"
             << setw(16) << tJacobi
             << setw(12) << iters << "\n";
        csv << n << ",Jacobi," << tJacobi << "," << iters << "\n";

        // --- Gauss ---
        t1 = high_resolution_clock::now();
        gaussEliminationSequential(A, b, x, n);
        t2 = high_resolution_clock::now();
        double tGauss = duration<double, milli>(t2 - t1).count();

        cout << left << setw(8) << n
             << setw(12) << "Gauss"
             << setw(16) << tGauss
             << setw(12) << "-" << "\n";
        csv << n << ",Gauss," << tGauss << ",1\n";

        cout << "\n";
    }

    csv.close();
    cout << "\nKết quả đã lưu vào: results_sequential.csv\n";
    return 0;
}
