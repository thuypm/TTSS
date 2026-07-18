#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <omp.h>
#include <iomanip>

using namespace std;

void printMatrixState(const vector<vector<double>>& A, const vector<double>& B) {
    int n = A.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            cout << setw(10) << setprecision(4) << fixed << A[i][j] << " ";
        }
        cout << "| " << setw(10) << setprecision(4) << fixed << B[i] << endl;
    }
    cout << "--------------------------------------------------------" << endl;
}

// Phương pháp khử Gauss có in từng bước
vector<double> solveGaussDebug(const vector<vector<double>>& A, const vector<double>& B) {
    int n = A.size();
    vector<vector<double>> tempA = A;
    vector<double> tempB = B;

    cout << "Ma tran bo sung ban dau [A|B]:" << endl;
    printMatrixState(tempA, tempB);

    for (int i = 0; i < n; ++i) {
        cout << "\n=== Buoc " << i + 1 << ": Khu cot " << i << " ===" << endl;
        // Chọn phần tử trội (partial pivoting)
        int pivot = i;
        for (int j = i + 1; j < n; ++j) {
            if (abs(tempA[j][i]) > abs(tempA[pivot][i])) {
                pivot = j;
            }
        }
        
        cout << "Gia tri lon nhat tren cot " << i << " ke tu dong " << i << " tro di la: " 
             << tempA[pivot][i] << " (dong " << pivot << ")" << endl;

        if (pivot != i) {
            cout << "Hoan vi dong " << i << " va dong " << pivot << endl;
            swap(tempA[i], tempA[pivot]);
            swap(tempB[i], tempB[pivot]);
            cout << "Ma tran sau khi hoan vi dong:" << endl;
            printMatrixState(tempA, tempB);
        } else {
            cout << "Khong can hoan vi dong." << endl;
        }

        if (abs(tempA[i][i]) < 1e-9) {
            throw runtime_error("Ma tran suy bien, khong the giai bang Gauss.");
        }

        // Khử xuôi song song hóa bằng OpenMP
        #pragma omp parallel for if(n >= 3)
        for (int j = i + 1; j < n; ++j) {
            double factor = tempA[j][i] / tempA[i][i];
            cout << "[Thread " << omp_get_thread_num() << "] Dong " << j 
                 << " = Dong " << j << " - (" << factor << ") * Dong " << i << endl;
            tempA[j][i] = 0.0;
            for (int k = i + 1; k < n; ++k) {
                tempA[j][k] -= factor * tempA[i][k];
            }
            tempB[j] -= factor * tempB[i];
        }

        cout << "Ma tran sau khi khu cot " << i << ":" << endl;
        printMatrixState(tempA, tempB);
    }

    // Thế ngược (tuần tự vì có tính phụ thuộc dữ liệu ngược)
    cout << "\n=== Giai doan the nguoc ===" << endl;
    vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        for (int j = i + 1; j < n; ++j) {
            sum += tempA[i][j] * x[j];
        }
        x[i] = (tempB[i] - sum) / tempA[i][i];
        cout << "Tính x[" << i + 1 << "] = (" << tempB[i] << " - " << sum << ") / " << tempA[i][i] 
             << " = " << x[i] << endl;
    }
    return x;
}

int main() {
    omp_set_num_threads(4);

    // Hệ phương trình 4x4 phức tạp hơn
    vector<vector<double>> A = {
        {1, -2, 3, -1},
        {2, 1, -1, 3},
        {-1, 3, 2, 2},
        {3, 2, -1, 1}
    };
    vector<double> B = {12, 3, 3, -2};

    try {
        vector<double> x = solveGaussDebug(A, B);

        cout << "\n--- KET QUA NGHIEM CUOI CUNG ---" << endl;
        for (size_t i = 0; i < x.size(); ++i) {
            cout << "x[" << i + 1 << "] = " << x[i] << endl;
        }
    } catch (const exception& e) {
        cerr << "Co loi: " << e.what() << endl;
    }

    return 0;
}
