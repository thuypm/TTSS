#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <omp.h>

using namespace std;

// Hàm sinh dữ liệu ngẫu nhiên cho vector để thử nghiệm
void generateRandomVector(vector<double>& v, int n) {
    for (int i = 0; i < n; i++) {
        v[i] = (rand() % 100) / 10.0; // Sinh số thực ngẫu nhiên từ 0.0 đến 9.9
    }
}

// 1. CHƯƠNG TRÌNH TUẦN TỰ (SEQUENTIAL)
double dotProductSequential(const vector<double>& a, const vector<double>& b, int n) {
    double result = 0.0;
    for (int i = 0; i < n; i++) {
        result += a[i] * b[i];
    }
    return result;
}

// 2. CHƯƠNG TRÌNH SONG SONG (OPENMP PARALLEL)
double dotProductParallel(const vector<double>& a, const vector<double>& b, int n) {
    double result = 0.0;

    // Sử dụng reduction(+:result) để các luồng tự tính tổng riêng rồi gộp lại an toàn
    #pragma omp parallel for reduction(+:result)
    for (int i = 0; i < n; i++) {
        result += a[i] * b[i];
    }
    return result;
}

// Hàm điều phối thử nghiệm cho từng kích thước kích thước N
void runExperiment(int N) {
    cout << "\n--------------------------------------------------" << endl;
    cout << "KICH THUOC THU NGHIEM (N): " << N << " phan tu" << endl;
    cout << "--------------------------------------------------" << endl;

    // Cấp phát bộ nhớ cho vector
    vector<double> a(N);
    vector<double> b(N);

    generateRandomVector(a, N);
    generateRandomVector(b, N);

    // --- Đo thời gian tuần tự ---
    double start_seq = omp_get_wtime();
    double res_seq = dotProductSequential(a, b, N);
    double end_seq = omp_get_wtime();
    double T_s = end_seq - start_seq;

    // --- Đo thời gian song song ---
    double start_par = omp_get_wtime();
    double res_par = dotProductParallel(a, b, N);
    double end_par = omp_get_wtime();
    double T_p = end_par - start_par;

    // Kiểm tra tính chính xác của thuật toán song song
    if (abs(res_seq - res_par) > 1e-5) {
        cout << "!!! CANH BAO: Ket qua hai phuong phap khong khop nhau!" << endl;
    }

    // Tính toán các chỉ số hiệu năng
    int P = omp_get_max_threads(); // Số luồng CPU tối đa đang sử dụng
    double S = T_s / T_p;          // Tốc độ tăng tiến (Speedup)
    double E = S / P;              // Hiệu suất sử dụng CPU (Efficiency)

    // In kết quả thống kê dạng bảng ngắn gọn
    cout << fixed << setprecision(6);
    cout << "-> Thoi gian Tuan tu (Ts)      : " << T_s << " giay" << endl;
    cout << "-> Thoi gian Song song (Tp)     : " << T_p << " giay" << endl;
    cout << "-> Toc do tang tien (Speedup S) : " << setprecision(2) << S << " lan (Tren " << P << " luong)" << endl;
    cout << "-> Hieu suat (Efficiency E)      : " << E * 100 << " %" << endl;
}

int main() {
    // Khởi tạo hạt giống sinh số ngẫu nhiên
    srand(time(0));

    cout << "=== THU NGHIEM DO HIEU NANG TICH VO HUONG (DOT PRODUCT) ===" << endl;

    // Thử nghiệm với các kích thước tăng dần từ nhỏ tới cực lớn
    runExperiment(1000);         // 1 Ngàn phần tử (Quá nhỏ)
    runExperiment(100000);       // 1 Trăm ngàn phần tử (Trung bình)
    runExperiment(10000000);     // 10 Triệu phần tử (Lớn)
    runExperiment(100000000);    // 100 Triệu phần tử (Cực lớn)

    return 0;
}