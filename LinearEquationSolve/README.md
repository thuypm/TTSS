# Linear Solver: Tuần tự vs Song song (OpenMP)

Giải hệ phương trình tuyến tính Ax = b bằng hai phương pháp:
- **Lặp Jacobi** — O(k·n²), dễ song song hóa
- **Khử Gauss có chọn pivot** — O(n³), song song hóa phần khử hàng

---

## Cấu trúc thư mục

```
linear_solver/
├── sequential.cpp       # Giải tuần tự (không dùng OpenMP)
├── parallel.cpp         # Giải song song (OpenMP)
├── CMakeLists.txt       # Build với CMake
├── run_benchmark.sh     # Script chạy benchmark tự động
├── dashboard.html       # Xem kết quả so sánh (mở bằng trình duyệt)
├── results_sequential.csv  # (sinh ra sau khi chạy)
├── results_parallel.csv    # (sinh ra sau khi chạy)
└── .vscode/
    └── tasks.json       # Tasks VS Code
```

---

## Cài đặt

### Yêu cầu
- GCC ≥ 9 (hỗ trợ C++17 và OpenMP)
- CMake ≥ 3.10 (tùy chọn)

### Ubuntu / WSL
```bash
sudo apt update
sudo apt install g++ cmake
```

### macOS (Homebrew)
```bash
brew install gcc libomp
# Dùng g++-13 thay vì g++ nếu cần
```

### Windows
- Cài [MSYS2](https://www.msys2.org/) → `pacman -S mingw-w64-ucrt-x86_64-gcc`
- Hoặc dùng WSL2 (Ubuntu) — khuyến nghị

---

## Build & Run

### Cách 1: Dùng VS Code Tasks (Ctrl+Shift+B)
Tasks đã được cấu hình trong `.vscode/tasks.json`:
- **Build All** — biên dịch cả sequential và parallel
- **Run Sequential** — chạy benchmark tuần tự
- **Run Parallel (OpenMP)** — chạy benchmark song song
- **Run Full Benchmark** — chạy toàn bộ + lưu CSV

### Cách 2: Dùng script
```bash
chmod +x run_benchmark.sh
./run_benchmark.sh
```

### Cách 3: Biên dịch thủ công
```bash
# Tuần tự
g++ -O3 -std=c++17 -o sequential sequential.cpp

# Song song (OpenMP)
g++ -O3 -std=c++17 -fopenmp -o parallel parallel.cpp

# Chạy
./sequential
./parallel
```

### Cách 4: CMake
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## Xem kết quả

Sau khi chạy, mở `dashboard.html` trong trình duyệt:

```bash
# Linux
xdg-open dashboard.html

# macOS
open dashboard.html

# Windows
start dashboard.html
```

Hoặc dùng Live Server extension trong VS Code.

---

## Công thức đánh giá

| Chỉ số | Công thức | Ý nghĩa |
|--------|-----------|---------|
| **Speedup** | S(p) = T_seq / T_par(p) | Tăng tốc bao nhiêu lần |
| **Efficiency** | E(p) = S(p) / p × 100% | % thời gian mỗi luồng làm việc thực sự |
| **Amdahl's Law** | S = 1 / (f + (1-f)/p) | Giới hạn lý thuyết, f = phần tuần tự |

---

## Phân tích kết quả

### Tại sao Jacobi dễ song song hóa hơn Gauss?
- **Jacobi**: Mỗi vòng lặp, tất cả xᵢ mới được tính từ x cũ → hoàn toàn độc lập → `#pragma omp parallel for`
- **Gauss**: Mỗi bước khử tạo data dependency với bước trước → chỉ song song hóa được phần khử trong một bước

### Khi nào OpenMP có lợi?
- N đủ lớn (N ≥ 500-1000) để công việc vượt overhead khởi tạo luồng
- Máy có nhiều core vật lý (≥ 4 cores)
- Với N nhỏ (N = 100-200): overhead thường lớn hơn speedup

### Độ phức tạp
- Jacobi: O(k·n²) — k ≈ 7-9 lần lặp hội tụ với ma trận chéo trội
- Gauss: O(n³) — tăng rất nhanh khi N lớn
