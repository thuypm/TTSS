#ifndef VECTOR_IO_H
#define VECTOR_IO_H

#include <bits/stdc++.h>

struct TestCase {
    int dimension;
    std::vector<double> vectorA;
    std::vector<double> vectorB;
};

struct TestResult {
    int dimension;
    std::vector<double> result;
};

std::vector<TestCase> readTestsFromFile(const std::string& fileName);
void writeResultsToFile(const std::string& fileName, const std::vector<TestResult>& results);

#endif
