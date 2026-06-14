#ifndef GAUSS_IO_H
#define GAUSS_IO_H

#include <bits/stdc++.h>

struct LinearSystemTest {
    std::string name;
    std::string method;
    int dimension;
    std::vector<std::vector<double>> matrixA;
    std::vector<double> vectorB;
    std::vector<double> expectedSolution;
};

std::vector<LinearSystemTest> readTestsFromFile(const std::string& fileName);

#endif
