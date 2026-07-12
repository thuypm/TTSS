#ifndef SOLVERS_H
#define SOLVERS_H

#include <vector>

// Giải hệ phương trình tuyến tính bằng phương pháp Cramer (Định thức)
std::vector<double> solveCramer(const std::vector<std::vector<double>>& A, const std::vector<double>& B);

// Giải hệ phương trình tuyến tính bằng phương pháp khử Gauss (có song song hóa OpenMP)
std::vector<double> solveGauss(const std::vector<std::vector<double>>& A, const std::vector<double>& B);

// Giải hệ phương trình tuyến tính bằng phương pháp lặp Jacobi
std::vector<double> solveJacobi(const std::vector<std::vector<double>>& A, const std::vector<double>& B, int maxIterations = 1000, double tolerance = 1e-6);

#endif // SOLVERS_H
