#include "solvers.h"
#include <cmath>
#include <stdexcept>
#include <omp.h>
#include <algorithm>
#include <iostream>

using namespace std;

vector<double> solveJacobiSequential(const vector<vector<double>>& A, const vector<double>& B, int maxIterations, double tolerance) {
    int n = A.size();
    if (n == 0 || A[0].size() != n || B.size() != n) {
        throw invalid_argument("Kich thuoc ma tran khong hop le.");
    }

    vector<double> x(n, 0.0);
    vector<double> x_new(n, 0.0);

    for (int iter = 0; iter < maxIterations; ++iter) {
        double max_diff = 0.0;
        
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    sum += A[i][j] * x[j];
                }
            }
            if (abs(A[i][i]) < 1e-12) {
                throw runtime_error("Ma tran co phan tu tren duong cheo chinh qua nho hoac bang 0.");
            }
            x_new[i] = (B[i] - sum) / A[i][i];
            max_diff = max(max_diff, abs(x_new[i] - x[i]));
        }

        x = x_new;
        if (max_diff < tolerance) {
            return x; // Hoi tu
        }
    }
    
    // Khong nem exception de benchmark van chay duoc voi ma tran lon
    // Chi in ra canh bao hoac tra ve gia tri tot nhat dat duoc
    cerr << "[Warning] Jacobi sequential khong hoi tu sau " << maxIterations << " vong lap." << endl;
    return x;
}

vector<double> solveJacobiParallel(const vector<vector<double>>& A, const vector<double>& B, int numThreads, int maxIterations, double tolerance) {
    int n = A.size();
    if (n == 0 || A[0].size() != n || B.size() != n) {
        throw invalid_argument("Kich thuoc ma tran khong hop le.");
    }

    vector<double> x(n, 0.0);
    vector<double> x_new(n, 0.0);
    omp_set_num_threads(numThreads);

    for (int iter = 0; iter < maxIterations; ++iter) {
        double max_diff = 0.0;

        #pragma omp parallel for reduction(max:max_diff)
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    sum += A[i][j] * x[j];
                }
            }
            
            x_new[i] = (B[i] - sum) / A[i][i];
            
            double diff = abs(x_new[i] - x[i]);
            if (diff > max_diff) {
                max_diff = diff;
            }
        }

        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            x[i] = x_new[i];
        }

        if (max_diff < tolerance) {
            return x; // Hoi tu
        }
    }

    cerr << "[Warning] Jacobi parallel khong hoi tu sau " << maxIterations << " vong lap." << endl;
    return x;
}
