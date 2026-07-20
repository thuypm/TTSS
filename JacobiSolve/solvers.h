#ifndef SOLVERS_H
#define SOLVERS_H

#include <vector>

// Giải hệ phương trình tuyến tính bằng phương pháp lặp Jacobi (Tuần tự)
std::vector<double> solveJacobiSequential(const std::vector<std::vector<double>>& A, const std::vector<double>& B, int maxIterations = 1000, double tolerance = 1e-6);

// Giải hệ phương trình tuyến tính bằng phương pháp lặp Jacobi (Song song OpenMP)
std::vector<double> solveJacobiParallel(const std::vector<std::vector<double>>& A, const std::vector<double>& B, int numThreads = 1, int maxIterations = 1000, double tolerance = 1e-6);

#endif // SOLVERS_H
