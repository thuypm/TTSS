# Báo cáo hiệu năng khử Gauss song song với OpenMP (Cập nhật cỡ mẫu 100 - 5000)

Dưới đây là kết quả đo lường thời gian thực thi, độ tăng tốc (Speedup) và hiệu suất song song (Efficiency) khi giải hệ phương trình tuyến tính bằng phương pháp khử Gauss song song sử dụng OpenMP với các số lượng CPU logic khác nhau: 1 luồng (tuần tự), 2 luồng, 8 luồng và 12 luồng (tối đa trên hệ thống hiện tại).

## 1. Bảng số liệu đo lường thực tế

| Kích cỡ N | T(1) (Seq) | T(2) | T(8) | T(12) | S(2) | S(8) | S(12) | E(2) | E(8) | E(12) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **100** | 0.0010s | 0.0038s | 0.0066s | 0.0086s | 0.26 | 0.15 | 0.12 | 13% | 2% | 1% |
| **200** | 0.0022s | 0.0080s | 0.0144s | 0.0186s | 0.28 | 0.15 | 0.12 | 14% | 2% | 1% |
| **400** | 0.0082s | 0.0186s | 0.0320s | 0.0396s | 0.44 | 0.26 | 0.21 | 22% | 3% | 2% |
| **800** | 0.0638s | 0.0616s | 0.0726s | 0.0880s | 1.03 | 0.88 | 0.73 | 52% | 11% | 6% |
| **1000** | 0.1182s | 0.0972s | 0.0996s | 0.1156s | 1.22 | 1.19 | 1.02 | 61% | 15% | 9% |
| **1600** | 0.5586s | 0.3396s | 0.2302s | 0.2434s | 1.64 | 2.43 | 2.29 | 82% | 30% | 19% |
| **3000** | 5.0388s | 3.6472s | 2.8190s | 2.4992s | 1.38 | 1.79 | 2.02 | 69% | 22% | 17% |
| **5000** | 20.2712s | 17.2466s | 16.2510s | 15.7060s | 1.18 | 1.25 | 1.29 | 59% | 16% | 11% |

> [!NOTE]
> * **Độ tăng tốc Speedup**: $S(p) = T_1 / T_p$
> * **Hiệu suất song song Efficiency**: $E(p) = S(p) / p$
> * Bộ test đã được tối ưu hóa lưu trữ: Các mẫu nhỏ ($N \le 800$) được gộp chung vào file `input.json` và `output.json`. Các mẫu lớn ($N \ge 1000$) được tách riêng thành từng file riêng biệt: `input_1000.json`, `input_1600.json`, `input_3000.json`, `input_5000.json` để tránh dung lượng file quá lớn.

---

## 2. Nhận xét và đánh giá hiệu năng
* **Kích thước nhỏ ($N \le 400$):**
  Phiên bản song song chạy **chậm hơn** đáng kể so với phiên bản tuần tự ($S(p) < 1.0$). Điều này là do khối lượng tính toán $O(N^3)$ lúc này quá nhỏ (chỉ mất dưới 10ms trên luồng đơn), trong khi chi phí (overhead) khởi tạo luồng và phân chia công việc của OpenMP lớn hơn rất nhiều so với thời gian tính toán thực tế.
* **Kích thước trung bình ($N = 800 - 1000$):**
  Thuật toán song song bắt đầu đạt điểm hòa vốn và bắt đầu nhanh hơn bản tuần tự ($S(2) \approx 1.03 - 1.22$). Tuy nhiên, cấu hình 8 và 12 luồng vẫn chậm hơn hoặc bằng tuần tự do số luồng lớn làm gia tăng chi phí đồng bộ hóa.
* **Kích thước lớn đạt đỉnh ($N = 1600$):**
  Hiệu năng song song cải thiện rõ rệt và đạt hiệu quả tối ưu nhất tại $N=1600$. Với $N=1600$, cấu hình 8 luồng đạt tốc độ nhanh nhất ($S(8) = 2.43$, giảm thời gian từ 0.56s xuống còn 0.23s). Ở điểm này, ma trận chiếm khoảng 20MB bộ nhớ, vẫn nằm trọn trong L3 Cache của CPU, giúp luồng truy xuất bộ nhớ siêu nhanh.
* **Hiện tượng suy sụt hiệu năng ở kích thước cực lớn ($N = 3000 - 5000$):**
  Khi tăng kích thước lên $N=3000$ (ma trận chiếm ~72MB) và $N=5000$ (ma trận chiếm ~200MB), **độ tăng tốc Speedup và hiệu suất sụt giảm mạnh** (với $N=5000$, Speedup của 8 luồng chỉ còn 1.25x và 12 luồng là 1.29x). 
  Hiện tượng này xảy ra do **Nghẽn băng thông bộ nhớ (Memory Bandwidth Bottleneck)** và **Tràn bộ nhớ đệm (Cache Thrashing)**. Khi dữ liệu ma trận vượt quá dung lượng L3 Cache của CPU (thường từ 16MB đến 32MB), các luồng phải liên tục truy xuất và ghi dữ liệu trực tiếp vào RAM (bộ nhớ ngoài). Khi tất cả các luồng đồng thời tranh chấp bus bộ nhớ để thực hiện phép trừ hàng, băng thông bộ nhớ bị nghẽn hoàn toàn, khiến các luồng CPU phải xếp hàng chờ đợi dữ liệu nạp từ RAM, triệt tiêu lợi ích của xử lý đa luồng.

---

## 3. Mã nguồn dùng cho Báo cáo LaTeX

Dưới đây là mã nguồn bảng số liệu và các biểu đồ bằng thư viện `pgfplots` để copy trực tiếp vào tài liệu báo cáo của bạn.

### A. Mã nguồn Bảng LaTeX (`booktabs`)
```latex
\begin{table}[h!]
  \centering
  \caption{Thời gian (giây), Speedup và Hiệu suất thực nghiệm khử Gauss}
  \label{tab:gauss_benchmark}
  \vspace{0.2cm}
  \small
  \begin{tabular}{r|rrrr|rrr|rrr}
    \toprule
    \textbf{N} & \multicolumn{4}{c|}{\textbf{Thời gian (s)}} & \multicolumn{3}{c|}{\textbf{Speedup $S(p)$}} & \multicolumn{3}{c}{\textbf{Hiệu suất $E(p)$}} \\
               & $p$=1 & $p$=2 & $p$=8 & $p$=12 & $S(2)$ & $S(8)$ & $S(12)$ & $E(2)$ & $E(8)$ & $E(12)$ \\
    \midrule
    100 & 0.0010 & 0.0038 & 0.0066 & 0.0086 & 0.26 & 0.15 & 0.12 & 0.13 & 0.02 & 0.01 \\
    200 & 0.0022 & 0.0080 & 0.0144 & 0.0186 & 0.28 & 0.15 & 0.12 & 0.14 & 0.02 & 0.01 \\
    400 & 0.0082 & 0.0186 & 0.0320 & 0.0396 & 0.44 & 0.26 & 0.21 & 0.22 & 0.03 & 0.02 \\
    800 & 0.0638 & 0.0616 & 0.0726 & 0.0880 & 1.03 & 0.88 & 0.73 & 0.52 & 0.11 & 0.06 \\
    1000 & 0.1182 & 0.0972 & 0.0996 & 0.1156 & 1.22 & 1.19 & 1.02 & 0.61 & 0.15 & 0.09 \\
    1600 & 0.5586 & 0.3396 & 0.2302 & 0.2434 & 1.64 & 2.43 & 2.29 & 0.82 & 0.30 & 0.19 \\
    3000 & 5.0388 & 3.6472 & 2.8190 & 2.4992 & 1.38 & 1.79 & 2.02 & 0.69 & 0.22 & 0.17 \\
    5000 & 20.2712 & 17.2466 & 16.2510 & 15.7060 & 1.18 & 1.25 & 1.29 & 0.59 & 0.16 & 0.11 \\
    \bottomrule
  \end{tabular}
\end{table}
```

### B. Biểu đồ Thời gian thực hiện (`pgfplots` / `tikzpicture`)
```latex
\begin{tikzpicture}
\begin{axis}[
  title={Thời gian thực hiện theo kích thước ma trận},
  xlabel={Kích thước ma trận $N$},
  ylabel={Thời gian (giây)},
  xtick={1,2,3,4,5,6,7,8},
  xticklabels={100, 200, 400, 800, 1000, 1600, 3000, 5000},
  ymode=log,
  log basis y=10,
  legend pos=north west,
  ymajorgrids=true,
  grid style=dashed,
  width=0.85\textwidth,
  height=6.5cm,
]
\addplot+[mark=circle*,thick,black] coordinates {(1,0.0010) (2,0.0022) (3,0.0082) (4,0.0638) (5,0.1182) (6,0.5586) (7,5.0388) (8,20.2712)};
\addlegendentry{Tuần tự ($p=1$)}
\addplot+[mark=square*,thick,blue] coordinates {(1,0.0038) (2,0.0080) (3,0.0186) (4,0.0618) (5,0.0972) (6,0.3396) (7,3.6472) (8,17.2466)};
\addlegendentry{Song song $p=2$}
\addplot+[mark=triangle*,thick,red] coordinates {(1,0.0066) (2,0.0144) (3,0.0320) (4,0.0726) (5,0.0996) (6,0.2302) (7,2.8190) (8,16.2510)};
\addlegendentry{Song song $p=8$}
\addplot+[mark=diamond*,thick,green!70!black] coordinates {(1,0.0086) (2,0.0186) (3,0.0396) (4,0.0880) (5,0.1156) (6,0.2434) (7,2.4992) (8,15.7060)};
\addlegendentry{Song song $p=12$}
\end{axis}
\end{tikzpicture}
```

### C. Biểu đồ Speedup LaTeX (`pgfplots` / `tikzpicture`)
```latex
\begin{tikzpicture}
\begin{axis}[
  title={Speedup theo kích thước ma trận},
  xlabel={Kích thước ma trận $N$},
  ylabel={Speedup $S(p) = T_1 / T_p$},
  xtick={1,2,3,4,5,6,7,8},
  xticklabels={100, 200, 400, 800, 1000, 1600, 3000, 5000},
  legend pos=north west,
  ymajorgrids=true,
  grid style=dashed,
  width=0.85\textwidth,
  height=6.5cm,
  ymin=0,
]
\addplot+[mark=square*,thick,blue] coordinates {(1,0.26) (2,0.28) (3,0.44) (4,1.03) (5,1.22) (6,1.64) (7,1.38) (8,1.18)};
\addlegendentry{$p=2$ luồng}
\addplot+[mark=triangle*,thick,red] coordinates {(1,0.15) (2,0.15) (3,0.26) (4,0.88) (5,1.19) (6,2.43) (7,1.79) (8,1.25)};
\addlegendentry{$p=8$ luồng}
\addplot+[mark=diamond*,thick,green!70!black] coordinates {(1,0.12) (2,0.12) (3,0.21) (4,0.73) (5,1.02) (6,2.29) (7,2.02) (8,1.29)};
\addlegendentry{$p=12$ luồng}
\addplot+[dashed,mark=none,blue!50] coordinates {(1,2) (8,2)};
\addlegendentry{Lý tưởng $p=2$}
\addplot+[dashed,mark=none,red!50] coordinates {(1,8) (8,8)};
\addlegendentry{Lý tưởng $p=8$}
\addplot+[dashed,mark=none,green!50] coordinates {(1,12) (8,12)};
\addlegendentry{Lý tưởng $p=12$}
\end{axis}
\end{tikzpicture}
```

### D. Biểu đồ Hiệu suất LaTeX (`pgfplots` / `tikzpicture`)
```latex
\begin{tikzpicture}
\begin{axis}[
  title={Hiệu suất song song},
  xlabel={Kích thước ma trận $N$},
  ylabel={Hiệu suất $E(p) = S(p)/p$},
  xtick={1,2,3,4,5,6,7,8},
  xticklabels={100, 200, 400, 800, 1000, 1600, 3000, 5000},
  ymin=0, ymax=1.2,
  legend pos=north west,
  ymajorgrids=true,
  grid style=dashed,
  width=0.85\textwidth,
  height=6.5cm,
]
\addplot+[mark=square*,thick,blue] coordinates {(1,0.13) (2,0.14) (3,0.22) (4,0.52) (5,0.61) (6,0.82) (7,0.69) (8,0.59)};
\addlegendentry{$p=2$ luồng}
\addplot+[mark=triangle*,thick,red] coordinates {(1,0.02) (2,0.02) (3,0.03) (4,0.11) (5,0.15) (6,0.30) (7,0.22) (8,0.16)};
\addlegendentry{$p=8$ luồng}
\addplot+[mark=diamond*,thick,green!70!black] coordinates {(1,0.01) (2,0.01) (3,0.02) (4,0.06) (5,0.09) (6,0.19) (7,0.17) (8,0.11)};
\addlegendentry{$p=12$ luồng}
\addplot+[dashed,mark=none,black,thick] coordinates {(1,1.0) (8,1.0)};
\addlegendentry{Lý tưởng ($E=1$)}
\end{axis}
\end{tikzpicture}
```

### E. Tối ưu hóa phẳng (1D Flat Array + Single Parallel Region)

Dưới đây là mã nguồn bảng LaTeX so sánh kết quả thực nghiệm tại $N=5000$:

```latex
\begin{table}[H]
  \centering
  \caption{Bảng so sánh thời gian thực thi (giây) giữa Bản gốc và Bản tối ưu phẳng ($N=5000$)}
  \label{tab:gauss_optimized_comparison}
  \vspace{0.2cm}
  \small
  \begin{tabular}{c|cc|c}
    \toprule
    \textbf{Số luồng $p$} & \textbf{Bản gốc (Original)} & \textbf{Tối ưu phẳng (Optimized)} & \textbf{Tốc độ cải thiện (\%)} \\
    \midrule
    $p=1$ (Seq) & 20.6480 & 20.8525 & -1.0\% \\
    $p=2$ & 17.9605 & 17.9170 & +0.2\% \\
    $p=8$ & 15.7935 & 15.6995 & +0.6\% \\
    $p=12$ & 15.6755 & 15.7165 & -0.3\% \\
    \bottomrule
  \end{tabular}
\end{table}
```

Dưới đây là mã nguồn 3 biểu đồ TikZ đường biểu diễn (Thời gian, Speedup, Efficiency) cho phần tối ưu phẳng:

```latex
% 1. Đồ thị Thời gian thực thi
\begin{figure}[H]
\centering
\begin{tikzpicture}
\begin{axis} [
  title={So sánh thời gian chạy thực nghiệm: Bản gốc vs Bản tối ưu ($N=5000$)},
  xlabel={Số lượng luồng $p$},
  ylabel={Thời gian (giây)},
  xtick={1,2,3,4},
  xticklabels={1, 2, 8, 12},
  legend pos=north east,
  ymajorgrids=true,
  grid style=dashed,
  width=0.85\textwidth,
  height=6.5cm,
  ymin=10, ymax=25,
]
\addplot+[mark=square*,thick,red] coordinates {(1, 20.6480) (2, 17.9605) (3, 15.7935) (4, 15.6755)};
\addlegendentry{Bản gốc (Original)}
\addplot+[mark=triangle*,thick,green!70!black] coordinates {(1, 20.8525) (2, 17.9170) (3, 15.6995) (4, 15.7165)};
\addlegendentry{Tối ưu phẳng (Optimized)}
\end{axis}
\end{tikzpicture}
\caption{Đồ thị so sánh thời gian thực thi giữa Bản gốc và Bản tối ưu phẳng}
\label{fig:gauss_optimized_comparison_chart}
\end{figure}

% 2. Đồ thị Speedup
\begin{figure}[H]
\centering
\begin{tikzpicture}
\begin{axis} [
  title={So sánh Speedup: Bản gốc vs Bản tối ưu phẳng ($N=5000$)},
  xlabel={Số lượng luồng $p$},
  ylabel={Độ tăng tốc Speedup $S(p)$},
  xtick={1,2,3,4},
  xticklabels={1, 2, 8, 12},
  legend pos=north west,
  ymajorgrids=true,
  grid style=dashed,
  width=0.85\textwidth,
  height=6.5cm,
  ymin=0, ymax=2,
]
\addplot+[mark=square*,thick,red] coordinates {(1, 1.0) (2, 1.15) (3, 1.31) (4, 1.32)};
\addlegendentry{Bản gốc (Original)}
\addplot+[mark=triangle*,thick,green!70!black] coordinates {(1, 1.0) (2, 1.16) (3, 1.33) (4, 1.33)};
\addlegendentry{Tối ưu phẳng (Optimized)}
\end{axis}
\end{tikzpicture}
\caption{Đồ thị so sánh độ tăng tốc Speedup giữa Bản gốc và Bản tối ưu phẳng}
\label{fig:gauss_optimized_speedup_chart}
\end{figure}

% 3. Đồ thị Hiệu suất song song
\begin{figure}[H]
\centering
\begin{tikzpicture}
\begin{axis} [
  title={So sánh Hiệu suất song song: Bản gốc vs Bản tối ưu phẳng ($N=5000$)},
  xlabel={Số lượng luồng $p$},
  ylabel={Hiệu suất $E(p)$},
  xtick={1,2,3,4},
  xticklabels={1, 2, 8, 12},
  legend pos=north east,
  ymajorgrids=true,
  grid style=dashed,
  width=0.85\textwidth,
  height=6.5cm,
  ymin=0, ymax=1.2,
]
\addplot+[mark=square*,thick,red] coordinates {(1, 1.0) (2, 0.58) (3, 0.16) (4, 0.11)};
\addlegendentry{Bản gốc (Original)}
\addplot+[mark=triangle*,thick,green!70!black] coordinates {(1, 1.0) (2, 0.58) (3, 0.17) (4, 0.11)};
\addlegendentry{Tối ưu phẳng (Optimized)}
\end{axis}
\end{tikzpicture}
\caption{Đồ thị so sánh hiệu suất song song giữa Bản gốc và Bản tối ưu phẳng}
\label{fig:gauss_optimized_efficiency_chart}
\end{figure}
```

### F. Tối ưu hóa nâng cao: Chia khối ma trận (Tiling/Blocked LU)

Dưới đây là mã nguồn bảng LaTeX so sánh cả 3 phiên bản tại $N=5000$:

```latex
\begin{table}[H]
  \centering
  \caption{Bảng so sánh thời gian thực thi (giây) của 3 thuật toán ($N=5000$)}
  \label{tab:gauss_blocked_comparison}
  \vspace{0.2cm}
  \small
  \begin{tabular}{c|ccc|c}
    \toprule
    \textbf{Threads $p$} & \textbf{Gốc (s)} & \textbf{Tối ưu phẳng (s)} & \textbf{Chia khối (s)} & \textbf{Tốc độ cải thiện cực đại} \\
    \midrule
    $p=1$ & 20.6480 & 20.8525 & 8.2120 & 2.51x \\
    $p=2$ & 17.9605 & 17.9170 & 4.4070 & 4.08x \\
    $p=8$ & 15.7935 & 15.6995 & 2.2880 & 6.90x \\
    $p=12$ & 15.6755 & 15.7165 & 1.9675 & 7.97x \\
    \bottomrule
  \end{tabular}
\end{table}
```

Dưới đây là mã nguồn 3 biểu đồ TikZ đường biểu diễn (Thời gian, Speedup, Efficiency) cho phần tối ưu chia khối ma trận:

```latex
% 1. Đồ thị Thời gian thực thi
\begin{figure}[H]
\centering
\begin{tikzpicture}
\begin{axis} [
  title={So sánh thời gian chạy thực nghiệm: Gốc vs Tối ưu phẳng vs Chia khối ($N=5000$)},
  xlabel={Số lượng luồng $p$},
  ylabel={Thời gian (giây)},
  xtick={1,2,3,4},
  xticklabels={1, 2, 8, 12},
  legend pos=north east,
  ymajorgrids=true,
  grid style=dashed,
  width=0.88\textwidth,
  height=7.0cm,
  ymin=0, ymax=25,
]
\addplot+[mark=square*,thick,red] coordinates {(1, 20.65) (2, 17.96) (3, 15.79) (4, 15.68)};
\addlegendentry{Bản gốc (Original)}
\addplot+[mark=triangle*,thick,blue] coordinates {(1, 20.85) (2, 17.92) (3, 15.70) (4, 15.72)};
\addlegendentry{Tối ưu phẳng (Optimized)}
\addplot+[mark=otimes*,thick,green!60!black] coordinates {(1, 8.21) (2, 4.41) (3, 2.29) (4, 1.97)};
\addlegendentry{Chia khối (Blocked, B=64)}
\end{axis}
\end{tikzpicture}
\caption{Biểu đồ thời gian thực thi so sánh thuật toán chia khối tại $N=5000$}
\label{fig:gauss_blocked_comparison_chart}
\end{figure}

% 2. Đồ thị Speedup
\begin{figure}[H]
\centering
\begin{tikzpicture}
\begin{axis} [
  title={So sánh Speedup: Gốc vs Tối ưu phẳng vs Chia khối ($N=5000$)},
  xlabel={Số lượng luồng $p$},
  ylabel={Độ tăng tốc Speedup $S(p)$},
  xtick={1,2,3,4},
  xticklabels={1, 2, 8, 12},
  legend pos=north west,
  ymajorgrids=true,
  grid style=dashed,
  width=0.88\textwidth,
  height=7.0cm,
  ymin=0, ymax=5,
]
\addplot+[mark=square*,thick,red] coordinates {(1, 1.0) (2, 1.15) (3, 1.31) (4, 1.32)};
\addlegendentry{Bản gốc (Original)}
\addplot+[mark=triangle*,thick,blue] coordinates {(1, 1.0) (2, 1.16) (3, 1.33) (4, 1.33)};
\addlegendentry{Tối ưu phẳng (Optimized)}
\addplot+[mark=otimes*,thick,green!60!black] coordinates {(1, 1.0) (2, 1.86) (3, 3.59) (4, 4.17)};
\addlegendentry{Chia khối (Blocked, B=64)}
\end{axis}
\end{tikzpicture}
\caption{Biểu đồ độ tăng tốc Speedup so sánh thuật toán chia khối tại $N=5000$}
\label{fig:gauss_blocked_speedup_chart}
\end{figure}

% 3. Đồ thị Hiệu suất song song
\begin{figure}[H]
\centering
\begin{tikzpicture}
\begin{axis} [
  title={So sánh Hiệu suất song song: Gốc vs Tối ưu phẳng vs Chia khối ($N=5000$)},
  xlabel={Số lượng luồng $p$},
  ylabel={Hiệu suất song song $E(p)$},
  xtick={1,2,3,4},
  xticklabels={1, 2, 8, 12},
  legend pos=north east,
  ymajorgrids=true,
  grid style=dashed,
  width=0.88\textwidth,
  height=7.0cm,
  ymin=0, ymax=1.2,
]
\addplot+[mark=square*,thick,red] coordinates {(1, 1.0) (2, 0.58) (3, 0.16) (4, 0.11)};
\addlegendentry{Bản gốc (Original)}
\addplot+[mark=triangle*,thick,blue] coordinates {(1, 1.0) (2, 0.58) (3, 0.17) (4, 0.11)};
\addlegendentry{Tối ưu phẳng (Optimized)}
\addplot+[mark=otimes*,thick,green!60!black] coordinates {(1, 1.0) (2, 0.93) (3, 0.45) (4, 0.35)};
\addlegendentry{Chia khối (Blocked, B=64)}
\end{axis}
\end{tikzpicture}
\caption{Biểu đồ hiệu suất song song so sánh thuật toán chia khối tại $N=5000$}
\label{fig:gauss_blocked_efficiency_chart}
\end{figure}
```


Dưới đây là mã nguồn C++ của phiên bản tối ưu hóa giải thuật khử Gauss sử dụng mảng phẳng 1D và vùng song song OpenMP đơn nhất:

```cpp
vector<double> solveGaussOptimized(const vector<double>& A_flat, const vector<double>& B, int n, int numThreads) {
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
            } // implicit barrier đồng bộ hóa trước khi khử

            // 2. Khử song song bằng omp for (không khởi tạo lại song song)
            #pragma omp for
            for (int j = i + 1; j < n; ++j) {
                double factor = tempA[j * n + i] / tempA[i * n + i];
                tempA[j * n + i] = 0.0;
                for (int k = i + 1; k < n; ++k) {
                    tempA[j * n + k] -= factor * tempA[i * n + k];
                }
                tempB[j] -= factor * tempB[i];
            } // implicit barrier đồng bộ hóa cho bước tiếp theo
        }
    }

    // 3. Thế ngược phẳng song song hóa reduction
    vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        #pragma omp parallel for reduction(+:sum) if(n - i - 1 >= 16)
        for (int j = i + 1; j < n; ++j) {
            sum += tempA[i * n + j] * x[j];
        }
        x[i] = (tempB[i] - sum) / tempA[i * n + i];
    }
    return x;
}
```

Dưới đây là mã nguồn C++ của phiên bản khử Gauss chia khối (Blocked LU) tối ưu hóa:

```cpp
vector<double> solveGaussBlocked(const vector<double>& A_flat, const vector<double>& B, int n, int numThreads, int blockSize) {
    vector<double> tempA = A_flat;
    vector<double> tempB = B;
    omp_set_num_threads(numThreads);

    #pragma omp parallel
    {
        for (int kb = 0; kb < n; kb += blockSize) {
            int end = min(kb + blockSize, n);

            // Pha 1 & Pha 2: Tuần tự - Factor panel và giải U12
            #pragma omp single
            {
                for (int i = kb; i < end; ++i) {
                    int pivot = i;
                    for (int j = i + 1; j < n; ++j) {
                        if (abs(tempA[j * n + i]) > abs(tempA[pivot * n + i])) pivot = j;
                    }
                    if (pivot != i) {
                        for (int k = 0; k < n; ++k) swap(tempA[i * n + k], tempA[pivot * n + k]);
                        swap(tempB[i], tempB[pivot]);
                    }

                    double pivotVal = tempA[i * n + i];
                    for (int j = i + 1; j < n; ++j) {
                        double factor = tempA[j * n + i] / pivotVal;
                        tempA[j * n + i] = factor;
                        for (int k = i + 1; k < end; ++k) {
                            tempA[j * n + k] -= factor * tempA[i * n + k];
                        }
                        tempB[j] -= factor * tempB[i];
                    }
                }

                for (int k = kb; k < end; ++k) {
                    for (int i = k + 1; i < end; ++i) {
                        double factor = tempA[i * n + k];
                        for (int j = end; j < n; ++j) {
                            tempA[i * n + j] -= factor * tempA[k * n + j];
                        }
                    }
                }
            } // Implicit barrier đồng bộ hóa trước khi cập nhật song song

            // Pha 3: Song song - Cập nhật khối A22 -= L21 * U12
            #pragma omp for schedule(static)
            for (int i = end; i < n; ++i) {
                for (int k = kb; k < end; ++k) {
                    double factor = tempA[i * n + k];
                    for (int j = end; j < n; ++j) {
                        tempA[i * n + j] -= factor * tempA[k * n + j];
                    }
                }
            } // Implicit barrier đồng bộ hóa trước khi qua panel tiếp theo
        }
    }

    // Thế ngược song song hóa reduction
    vector<double> x(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = 0.0;
        #pragma omp parallel for reduction(+:sum) if(n - i - 1 >= 16)
        for (int j = i + 1; j < n; ++j) {
            sum += tempA[i * n + j] * x[j];
        }
        x[i] = (tempB[i] - sum) / tempA[i * n + i];
    }
    return x;
}
```
