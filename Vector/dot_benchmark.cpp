#include <bits/stdc++.h>
#include <omp.h>

using namespace std;

// Tinh tich vo huong song song dung reduction
double dotProduct(const vector<double>& a, const vector<double>& b, int numThreads) {
    int n = static_cast<int>(a.size());
    double sum = 0.0;
    omp_set_num_threads(numThreads);
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

int main() {
    // Cac kich thuoc vector can do
    vector<int> sizes = {1000000, 5000000, 10000000, 50000000, 100000000};
    // Cac so luong thread can do
    vector<int> threadCounts = {1, 2, 8};
    int REPEATS = 5; // So lan lap de lay trung binh

    cout << "Bat dau benchmark tich vo huong..." << endl;

    // Mo file JSON de ghi ket qua
    ofstream out("benchmark_results.json");
    out << fixed << setprecision(9);
    out << "{\n  \"results\": [\n";

    bool firstSize = true;
    for (int n : sizes) {
        cout << "  Kich thuoc n = " << n << " ..." << endl;

        // Sinh du lieu ngau nhien
        mt19937 rng(42);
        uniform_real_distribution<double> dist(1.0, 100.0);
        vector<double> A(n), B(n);
        for (int i = 0; i < n; i++) { A[i] = dist(rng); B[i] = dist(rng); }

        if (!firstSize) out << ",\n";
        firstSize = false;
        out << "    {\n";
        out << "      \"n\": " << n << ",\n";
        out << "      \"times\": {\n";

        bool firstThread = true;
        for (int t : threadCounts) {
            // Warm-up
            dotProduct(A, B, t);

            // Do thoi gian
            double totalTime = 0.0;
            double result = 0.0;
            for (int r = 0; r < REPEATS; r++) {
                double t0 = omp_get_wtime();
                result = dotProduct(A, B, t);
                double t1 = omp_get_wtime();
                totalTime += (t1 - t0);
            }
            double avgTime = totalTime / REPEATS;
            cout << "    " << t << " thread(s): " << avgTime << "s  (result=" << result << ")" << endl;

            if (!firstThread) out << ",\n";
            firstThread = false;
            out << "        \"" << t << "\": " << avgTime;
        }
        out << "\n      }\n    }";
    }

    out << "\n  ]\n}\n";
    out.close();

    cout << "Hoan thanh! Ket qua da ghi vao benchmark_results.json" << endl;
    return 0;
}
