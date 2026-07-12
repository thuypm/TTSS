#include "solvers.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <omp.h>

// Hàm tính định thức bằng biến đổi Gauss
double calculateDeterminant(std::vector<std::vector<double>> matrix) {
    int n = matrix.size();
    double det = 1.0;
    for (int i = 0; i < n; ++i) {
        int pivot = i;
        for (int j = i + 1; j < n; ++j) {
            if (std::abs(matrix[j][i]) > std::abs(matrix[pivot][i])) {
                pivot = j;
            }
        }
        if (pivot != i) {
            std::swap(matrix[i], matrix[pivot]);
            det *= -1.0;
        }
        if (std::abs(matrix[i][i]) < 1e-9) {
            return 0.0;
        }
        det *= matrix[i][i];
        for (int j = i + 1; j < n; ++j) {
            double factor = matrix[j][i] / matrix[i][i];
            for (int k = i; k < n; ++k) {
                matrix[j][k] -= factor * matrix[i][k];
            }
        }
    }
    return det;
}

// Phương pháp Cramer
std::vector<double> solveCramer(const std::vector<std::vector<double>>& A, const std::vector<double>& B) {
    int n = A.size();
    double detA = calculateDeterminant(A);
    if (std::abs(detA) < 1e-9) {
        throw std::runtime_error("Ma tran suy bien (det = 0), khong the giai bang Cramer.");
    }
    std::vector<double> x(n);
    for (int i = 0; i < n; ++i) {
        std::vector<std::vector<double>> Ai = A;
        for (int j = 0; j < n; ++j) {
            Ai[j][i] = B[j];
        }
        x[i] = calculateDeterminant(Ai) / detA;
    }
    return x;
}

// Phương pháp khử Gauss (song song hóa bằng OpenMP)
std::vector<double> solveGauss(const std::vector<std::vector<double>>& A, const std::vector<double>& B) {
    int n = A.size();
    std::vector<std::vector<double>> tempA = A;
    std::vector<double> tempB = B;

    for (int i = 0; i < n; ++i) {
        // Chọn phần tử trội (partial pivoting)
        int pivot = i;
        for (int j = i + 1; j < n; ++j) {
            if (std::abs(tempA[j][i]) > std::abs(tempA[pivot][i])) {
                pivot = j;
            }
        }
        if (pivot != i) {
            std::swap(tempA[i], tempA[pivot]);
            std::swap(tempB[i], tempB[pivot]);
        }
        if (std::abs(tempA[i][i]) < 1e-9) {
            throw std::runtime_error("Ma tran suy bien, khong the giai bang Gauss.");
        }

        // Khử xuôi song song hóa bằng OpenMP
        #pragma omp parallel for if(n >= 3)
        for (int j = i + 1; j < n; ++j) {
            double factor = tempA[j][i] / tempA[i][i];
            tempA[j][i] = 0.0;
            for (int k = i + 1; k < n; ++k) {
                tempA[j][k] -= factor * tempA[i][k];
            }
            tempB[j] -= factor * tempB[i];
        }
    }

    // Thế ngược (tuần tự vì có tính phụ thuộc dữ liệu ngược)
    std::vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        for (int j = i + 1; j < n; ++j) {
            sum += tempA[i][j] * x[j];
        }
        x[i] = (tempB[i] - sum) / tempA[i][i];
    }
    return x;
}

// Phương pháp lặp Jacobi
std::vector<double> solveJacobi(const std::vector<std::vector<double>>& A, const std::vector<double>& B, int maxIterations, double tolerance) {
    int n = A.size();
    std::vector<double> x(n, 0.0);
    std::vector<double> x_new(n, 0.0);

    for (int i = 0; i < n; ++i) {
        if (std::abs(A[i][i]) < 1e-9) {
            throw std::runtime_error("Duong cheo chinh co phan tu bang 0, khong the giai bang Jacobi.");
        }
    }

    for (int iter = 0; iter < maxIterations; ++iter) {
        #pragma omp parallel for if(n >= 3)
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    sum += A[i][j] * x[j];
                }
            }
            x_new[i] = (B[i] - sum) / A[i][i];
        }

        // Tính sai số hội tụ tối đa (L-infinity norm)
        double diff = 0.0;
        for (int i = 0; i < n; ++i) {
            diff = std::max(diff, std::abs(x_new[i] - x[i]));
        }

        x = x_new;
        if (diff < tolerance) {
            break;
        }
    }
    return x;
}
