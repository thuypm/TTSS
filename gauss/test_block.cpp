#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <random>
#include <vector>


using namespace std;

// 1. Phép giải Gauss gốc (2D vector, parallel for lặp lại)
vector<double> solveGaussOriginal(const vector<vector<double>> &A,
                                  const vector<double> &B, int numThreads) {
  int n = A.size();
  vector<vector<double>> tempA = A;
  vector<double> tempB = B;

  omp_set_num_threads(numThreads);

  for (int i = 0; i < n; ++i) {
    int pivot = i;
    for (int j = i + 1; j < n; ++j) {
      if (abs(tempA[j][i]) > abs(tempA[pivot][i])) {
        pivot = j;
      }
    }
    if (pivot != i) {
      swap(tempA[i], tempA[pivot]);
      swap(tempB[i], tempB[pivot]);
    }

#pragma omp parallel for if (n - i >= 16)
    for (int j = i + 1; j < n; ++j) {
      double factor = tempA[j][i] / tempA[i][i];
      tempA[j][i] = 0.0;
      for (int k = i + 1; k < n; ++k) {
        tempA[j][k] -= factor * tempA[i][k];
      }
      tempB[j] -= factor * tempB[i];
    }
  }

  vector<double> x(n);
  for (int i = n - 1; i >= 0; --i) {
    double sum = 0.0;
#pragma omp parallel for reduction(+ : sum) if (n - i - 1 >= 16)
    for (int j = i + 1; j < n; ++j) {
      sum += tempA[i][j] * x[j];
    }
    x[i] = (tempB[i] - sum) / tempA[i][i];
  }
  return x;
}

// 2. Phép giải Gauss tối ưu (Mảng phẳng 1D + Một vùng song song duy nhất)
vector<double> solveGaussOptimized(const vector<double> &A_flat,
                                   const vector<double> &B, int n,
                                   int numThreads) {
  vector<double> tempA = A_flat;
  vector<double> tempB = B;

  omp_set_num_threads(numThreads);

#pragma omp parallel
  {
    for (int i = 0; i < n; ++i) {
// 1. Chọn phần tử trội tuần tự trên luồng Single
#pragma omp single
      {
        int pivot = i;
        for (int j = i + 1; j < n; ++j) {
          if (abs(tempA[j * n + i]) > abs(tempA[pivot * n + i])) {
            pivot = j;
          }
        }
        if (pivot != i) {
          for (int k = 0; k < n; ++k) {
            swap(tempA[i * n + k], tempA[pivot * n + k]);
          }
          swap(tempB[i], tempB[pivot]);
        }
      } // Rào chắn ngầm định tại đây giúp đồng bộ hóa tất cả các luồng trước
        // khi bắt đầu khử

// 2. Khử hàng song song sử dụng omp for (không khởi tạo lại vùng song song)
#pragma omp for
      for (int j = i + 1; j < n; ++j) {
        double factor = tempA[j * n + i] / tempA[i * n + i];
        tempA[j * n + i] = 0.0;
        for (int k = i + 1; k < n; ++k) {
          tempA[j * n + k] -= factor * tempA[i * n + k];
        }
        tempB[j] -= factor * tempB[i];
      } // Rào chắn ngầm định đồng bộ các luồng cho bước tiếp theo
    }
  }

  // 3. Thế ngược (Back-substitution) phẳng
  vector<double> x(n);
  for (int i = n - 1; i >= 0; --i) {
    double sum = 0.0;
#pragma omp parallel for reduction(+ : sum) if (n - i - 1 >= 16)
    for (int j = i + 1; j < n; ++j) {
      sum += tempA[i * n + j] * x[j];
    }
    x[i] = (tempB[i] - sum) / tempA[i * n + i];
  }
  return x;
}

// 3. Phép giải Gauss CHIA BLOCK (Tiling / Blocked LU kieu LAPACK dgetrf)
//    - Mang phang 1D, 1 vung song song duy nhat (giong ban Optimized)
//    - Chia ma tran thanh cac panel rong blockSize cot:
//        Pha 1: Factor hoa panel (tuan tu) - chi cap nhat trong pham vi panel
//        (re) Pha 2: Giai tam giac U12 cho cac hang panel tren phan cot con lai
//        (tuan tu, re) Pha 3: Cap nhat khoi A22 -= L21 * U12 (SONG SONG, day la
//        phan ton 90% thoi gian)
//    Loi ich: khi lam Pha 3, du lieu panel (L21, U12) nho, nam gon trong Cache
//    L2, duoc tai su dung nhieu lan truoc khi bi day ra RAM -> giam Cache
//    Thrashing.
vector<double> solveGaussBlocked(const vector<double> &A_flat,
                                 const vector<double> &B, int n, int numThreads,
                                 int blockSize) {
  vector<double> tempA = A_flat;
  vector<double> tempB = B;

  omp_set_num_threads(numThreads);

#pragma omp parallel
  {
    for (int kb = 0; kb < n; kb += blockSize) {
      int end = min(kb + blockSize, n);

// ---------- Pha 1 + Pha 2: chi 1 luong thuc hien (co phu thuoc du lieu tuan
// tu) ----------
#pragma omp single
      {
        // ----- Pha 1: Factor hoa panel (cot kb..end-1) -----
        for (int i = kb; i < end; ++i) {
          // Chon phan tu troi trong cot i
          int pivot = i;
          for (int j = i + 1; j < n; ++j) {
            if (abs(tempA[j * n + i]) > abs(tempA[pivot * n + i]))
              pivot = j;
          }
          if (pivot != i) {
            for (int k = 0; k < n; ++k)
              swap(tempA[i * n + k], tempA[pivot * n + k]);
            swap(tempB[i], tempB[pivot]);
          }

          double pivotVal = tempA[i * n + i];
          for (int j = i + 1; j < n; ++j) {
            double factor = tempA[j * n + i] / pivotVal;
            tempA[j * n + i] = factor; // luu he so nhan (L21) de dung o Pha 3
            // CHI cap nhat trong pham vi panel (khac voi ban Optimized: cap
            // nhat het n-1)
            for (int k = i + 1; k < end; ++k) {
              tempA[j * n + k] -= factor * tempA[i * n + k];
            }
            tempB[j] -= factor * tempB[i];
          }
        }

        // ----- Pha 2: Tinh U12 - ap phep khu cua panel len cac cot con lai,
        // -----
        // ----- nhung chi cho cac HANG thuoc panel (kb..end-1) -----
        for (int k = kb; k < end; ++k) {
          for (int i = k + 1; i < end; ++i) {
            double factor = tempA[i * n + k];
            for (int j = end; j < n; ++j) {
              tempA[i * n + j] -= factor * tempA[k * n + j];
            }
          }
        }
      } // <- Rao chan ngam dinh: cac luong khac cho toi day moi chay tiep

// ---------- Pha 3: A22 -= L21 * U12 (SONG SONG, khong khoi tao lai vung song
// song) ----------
#pragma omp for schedule(static)
      for (int i = end; i < n; ++i) {
        for (int k = kb; k < end; ++k) {
          double factor = tempA[i * n + k];
          for (int j = end; j < n; ++j) {
            tempA[i * n + j] -= factor * tempA[k * n + j];
          }
        }
      } // <- Rao chan ngam dinh truoc khi sang panel (kb) tiep theo
    }
  }

  // Thay nguoc (Back-substitution) - giong het ban Optimized
  vector<double> x(n);
  for (int i = n - 1; i >= 0; --i) {
    double sum = 0.0;
#pragma omp parallel for reduction(+ : sum) if (n - i - 1 >= 16)
    for (int j = i + 1; j < n; ++j) {
      sum += tempA[i * n + j] * x[j];
    }
    x[i] = (tempB[i] - sum) / tempA[i * n + i];
  }
  return x;
}

int main() {
  int n = 5000;
  int blockSize =
      64; // B = 64x64 -> vua khop L2 Cache theo phan tich trong tai lieu
  vector<int> threadCounts = {1, 2, 8, 12};
  int REPEATS = 2; // Lặp 2 lần để lấy trung bình nhanh hơn cho N=5000

  cout << "Kich thuoc thu nghiem so sanh N = " << n << endl;

  // Sinh ma trận ngẫu nhiên trội chéo
  mt19937 rng(42);
  uniform_real_distribution<double> distVal(-10.0, 10.0);
  uniform_real_distribution<double> distDiag(5.0, 20.0);

  vector<vector<double>> A(n, vector<double>(n));
  vector<double> A_flat(n * n);
  vector<double> B(n);

  for (int i = 0; i < n; ++i) {
    double rowSum = 0.0;
    for (int j = 0; j < n; ++j) {
      if (i != j) {
        double val = distVal(rng);
        A[i][j] = val;
        A_flat[i * n + j] = val;
        rowSum += abs(val);
      }
    }
    double diag = (distVal(rng) >= 0 ? 1.0 : -1.0) * (rowSum + distDiag(rng));
    A[i][i] = diag;
    A_flat[i * n + i] = diag;
    B[i] = distVal(rng) * 10.0;
  }

  // Kiểm tra tính đúng đắn trước
  cout << "Kiem tra tinh dung dan..." << endl;
  vector<double> x_orig = solveGaussOriginal(A, B, 8);
  vector<double> x_opt = solveGaussOptimized(A_flat, B, n, 8);
  vector<double> x_blk = solveGaussBlocked(A_flat, B, n, 8, blockSize);

  double maxDiff = 0.0;
  for (int i = 0; i < n; ++i) {
    maxDiff = max(maxDiff, abs(x_orig[i] - x_opt[i]));
  }
  cout << "Max difference (Original vs Optimized): " << maxDiff
       << (maxDiff < 1e-9 ? " (PASSED)" : " (FAILED)") << endl;

  double maxDiffBlk = 0.0;
  for (int i = 0; i < n; ++i) {
    maxDiffBlk = max(maxDiffBlk, abs(x_orig[i] - x_blk[i]));
  }
  // Luu y: thu tu cong/tru dau phay dong khac nhau giua ban Blocked va ban tuan
  // tu co the tao sai so lam tron rat nho (thuong < 1e-7 voi N=5000), nen
  // nguong so sanh duoc noi long hon mot chut so voi phep so sanh Original vs
  // Optimized.
  cout << "Max difference (Original vs Blocked):   " << maxDiffBlk
       << (maxDiffBlk < 1e-6 ? " (PASSED)" : " (FAILED)") << endl;

  // Đo thời gian bản gốc
  cout << "Bat dau do ban Goc (Original)..." << endl;
  vector<double> timesOrig;
  for (int t : threadCounts) {
    // Warmup
    solveGaussOriginal(A, B, t);

    double totalTime = 0.0;
    for (int r = 0; r < REPEATS; ++r) {
      double t0 = omp_get_wtime();
      solveGaussOriginal(A, B, t);
      double t1 = omp_get_wtime();
      totalTime += (t1 - t0);
    }
    double avgTime = totalTime / REPEATS;
    cout << "  " << t << " thread(s): " << avgTime << "s" << endl;
    timesOrig.push_back(avgTime);
  }

  // Đo thời gian bản tối ưu
  cout << "Bat dau do ban Toi uu (Optimized)..." << endl;
  vector<double> timesOpt;
  for (int t : threadCounts) {
    // Warmup
    solveGaussOptimized(A_flat, B, n, t);

    double totalTime = 0.0;
    for (int r = 0; r < REPEATS; ++r) {
      double t0 = omp_get_wtime();
      solveGaussOptimized(A_flat, B, n, t);
      double t1 = omp_get_wtime();
      totalTime += (t1 - t0);
    }
    double avgTime = totalTime / REPEATS;
    cout << "  " << t << " thread(s): " << avgTime << "s" << endl;
    timesOpt.push_back(avgTime);
  }

  // Đo thời gian bản chia block (Blocked / Tiled)
  cout << "Bat dau do ban Chia Block (Blocked, B=" << blockSize << ")..."
       << endl;
  vector<double> timesBlk;
  for (int t : threadCounts) {
    // Warmup
    solveGaussBlocked(A_flat, B, n, t, blockSize);

    double totalTime = 0.0;
    for (int r = 0; r < REPEATS; ++r) {
      double t0 = omp_get_wtime();
      solveGaussBlocked(A_flat, B, n, t, blockSize);
      double t1 = omp_get_wtime();
      totalTime += (t1 - t0);
    }
    double avgTime = totalTime / REPEATS;
    cout << "  " << t << " thread(s): " << avgTime << "s" << endl;
    timesBlk.push_back(avgTime);
  }

  // Ghi file so sanh json
  ofstream file("opt_data.json");
  file << fixed << setprecision(6);
  file << "{\n";
  file << "  \"n\": " << n << ",\n";
  file << "  \"threads\": [";
  for (size_t i = 0; i < threadCounts.size(); ++i) {
    file << threadCounts[i];
    if (i + 1 < threadCounts.size())
      file << ", ";
  }
  file << "],\n";
  file << "  \"original\": [";
  for (size_t i = 0; i < timesOrig.size(); ++i) {
    file << timesOrig[i];
    if (i + 1 < timesOrig.size())
      file << ", ";
  }
  file << "],\n";
  file << "  \"optimized\": [";
  for (size_t i = 0; i < timesOpt.size(); ++i) {
    file << timesOpt[i];
    if (i + 1 < timesOpt.size())
      file << ", ";
  }
  file << "],\n";
  file << "  \"blockSize\": " << blockSize << ",\n";
  file << "  \"blocked\": [";
  for (size_t i = 0; i < timesBlk.size(); ++i) {
    file << timesBlk[i];
    if (i + 1 < timesBlk.size())
      file << ", ";
  }
  file << "]\n";
  file << "}\n";
  file.close();
  cout << "Da ghi file opt_data.json!" << endl;

  return 0;
}