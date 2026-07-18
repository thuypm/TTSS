# Tài liệu Phân tích và Hướng Tối ưu hóa Thuật toán Khử Gauss Song song với OpenMP

Tài liệu này tổng hợp các phân tích chuyên sâu về phần cứng, các nút cổ chai hiệu năng thực tế, và các giải pháp tối ưu hóa kiến trúc bộ nhớ đối với giải thuật khử Gauss song song.

---

## 1. Các Nút Cổ Chai Hiệu Năng Chính (Bottlenecks)

Khi chạy khử Gauss trên các ma trận lớn (như $N=3000$ chiếm ~72MB, hoặc $N=5000$ chiếm ~200MB), hiệu năng đa luồng bị giới hạn mạnh bởi 3 nguyên nhân cốt lõi:

### 1.1. Hiện tượng Tràn Bộ Nhớ Đệm L3 (Cache Thrashing)
* **Thực trạng**: Kích thước ma trận $5000 \times 5000$ kiểu số thực `double` (8 bytes) chiếm khoảng **200 MB** trong bộ nhớ. Trong khi đó, bộ nhớ đệm L3 Cache của các CPU hiện đại dùng chung cho các nhân chỉ từ **16MB đến 32MB**.
* **Hậu quả**: Do ma trận quá lớn vượt quá dung lượng L3 Cache nhiều lần, các khối dữ liệu cũ liên tục bị CPU xóa bỏ để nhường chỗ cho dữ liệu mới nạp từ RAM. Chu trình tráo đổi liên tục này gọi là **Cache Thrashing**, khiến độ trễ truy xuất dữ liệu tăng từ ~10 chu kỳ xung nhịp (khi nằm trong Cache) lên ~200-300 chu kỳ xung nhịp (khi phải đọc/ghi từ RAM vật lý).

### 1.2. Nghẽn Băng Thông Bộ Nhớ RAM (Memory Bandwidth Bottleneck)
* **Thực trạng**: Khử Gauss là bài toán có **Cường độ tính toán (Arithmetic Intensity) cực kỳ thấp**. Với mỗi phần tử ma trận (8 bytes) nạp từ RAM vào thanh ghi CPU, luồng chỉ thực hiện đúng một phép nhân và một phép trừ (`-= factor * A`). Tỷ lệ Flops/Byte chỉ khoảng 0.25.
* **Hậu quả**: CPU dành 90% thời gian hoạt động chỉ để xếp hàng chờ dữ liệu nạp/ghi về RAM. Khi tăng số lượng luồng tính toán song song, các luồng đồng thời tranh chấp đường truyền bộ nhớ dùng chung (**Memory Bus**), gây nghẽn hoàn toàn băng thông RAM của bo mạch chủ, khiến tốc độ Speedup của cấu hình 12 luồng không cải thiện nhiều so với 1 hay 2 luồng.

### 1.3. Chi Phí Lặp Khởi Tạo Vùng Song Song (Thread Fork-Join Overhead)
* Vòng lặp khử cột bên ngoài chạy $N$ lần. Nếu sử dụng `#pragma omp parallel for` ở mỗi bước, OpenMP phải mở và đóng vùng song song $N$ lần.
* Với $N=5000$, OpenMP thực hiện Fork-Join 5000 lần, tích lũy một lượng chi phí quản lý luồng đáng kể làm giảm gia tốc Speedup.

---

## 2. Các Giải Pháp Tối Ưu Hóa & Đánh Đổi (Trade-offs)

### 2.1. Vùng Song Song Đơn Nhất (Single Parallel Region)
* **Giải pháp**: Mở `#pragma omp parallel` duy nhất 1 lần bao trùm toàn bộ vòng lặp lớn. Phối hợp giữa `#pragma omp single` (để 1 luồng thực hiện Pivoting tuần tự) và `#pragma omp for` (để nhiều luồng tính toán khử song song).
* **Kết quả**: Triệt tiêu hoàn toàn chi phí khởi tạo lại luồng lặp đi lặp lại $N$ lần.

### 2.2. Mảng 1D Phẳng (Flat 1D Array)
* **Giải pháp**: Chuyển đổi ma trận hệ số từ `vector<vector<double>>` phân mảnh trên Heap sang mảng một chiều liên tục `vector<double> A_flat(n * n)`.
* **Ưu điểm**: Nâng cao tính cục bộ không gian (Spatial Locality), hỗ trợ tối đa bộ nạp trước của CPU (Hardware Prefetcher) và giúp trình biên dịch tự động thực hiện vector hóa SIMD (AVX2/AVX-512).
* **Đánh đổi về hàm `swap` (Pivoting)**:
  * *Trong Bản gốc (2D Vector)*: Phép đổi hàng `swap(A[i], A[pivot])` chỉ tráo đổi 3 con trỏ quản lý của vector nên có độ phức tạp **$O(1)$** (chỉ tốn dưới 1 nanosecond).
  * *Trong Bản tối ưu 1D*: Vì chung mảng phẳng, ta phải chạy vòng lặp $N$ lần để tráo đổi từng phần tử double đơn lẻ. Độ phức tạp tăng lên **$O(N)$** (tốn khoảng vài microsecond cho $N=5000$).
  * *Hệ quả*: Thời gian tiết kiệm được từ việc khử song song 1D phẳng bị bù trừ một phần bởi khâu đổi hàng tuần tự chậm đi.

---

## 3. Bản Chất của Giải Pháp Chia Khối Ma Trận (Tiling / Block LU)

Đối với các hệ phương trình tuyến tính cực lớn, giải pháp triệt để nhất để vượt qua giới hạn băng thông RAM là **Tiling/Blocking**.

### 3.1. Tại sao Chia Khối lại khả thi dù vẫn phải nạp dữ liệu từ RAM?
Việc nạp dữ liệu từ RAM vào Cache và ghi ngược lại hoàn toàn do **phần cứng CPU tự động thực hiện** mỗi khi tính toán. Sự vượt trội của chia khối đến từ **Khả năng tái sử dụng dữ liệu trong Cache (Cache Reuse / Temporal Locality)**:

* **Không chia khối (Quét dòng tuần tự)**: Dòng $j$ được nạp vào Cache $\rightarrow$ Khử xong cột $i$ $\rightarrow$ Bị đẩy ngay ra RAM $\rightarrow$ Đến bước khử cột $i+1$ lại phải đọc RAM nạp lại dòng $j$. Tỷ lệ Đọc RAM / Số phép toán là $1:1$.
* **Có chia khối (Tiling)**: Ta nạp một khối con vuông kích thước $B \times B$ vào Cache. CPU thực hiện **toàn bộ 64 bước khử cục bộ** ngay trên khối con đang nằm trong Cache L2/L3 siêu nhanh mà không truy cập RAM ngoài. Chỉ sau khi xong toàn bộ 64 bước, ta mới ghi kết quả khối con về RAM **1 lần duy nhất**. Tỷ lệ đọc ghi RAM giảm đi $B$ lần (ví dụ giảm 64 lần).

### 3.2. Ví dụ Minh họa Trực quan
* **Không chia khối**: Giống như thợ xây mỗi lần cần đặt 1 viên gạch xây tường lại phải đi bộ 100m ra kho chứa gạch để lấy đúng 1 viên về đặt. Thời gian đi bộ (Memory Latency) chiếm 95% thời gian làm việc.
* **Có chia khối**: Giống như thợ xây dùng xe rùa chở một lần 64 viên gạch về đặt ngay cạnh chân công trình. Thợ xây đứng một chỗ lấy gạch từ xe rùa xây liên tục 64 viên cực nhanh. Khi hết sạch gạch trên xe mới ra kho chở chuyến tiếp theo.

### 3.3. Lựa Chọn Kích Thước Khối ($B$) Tối Ưu
Kích thước khối con $B \times B$ được thiết kế để nằm trọn trong **L2 Cache** (thường từ 256KB đến 1MB mỗi nhân):
* **$B = 64$**: Một khối số thực double $64 \times 64$ chiếm khoảng **32 KB**. Ba khối (Chốt, Hàng bên phải, Cột bên dưới) dùng trong phép khử chiếm tổng cộng **96 KB** $\rightarrow$ Nằm hoàn hảo trong L2 Cache, cho tốc độ truy xuất cực đại.
* **$B = 128$**: Ba khối chiếm **384 KB** $\rightarrow$ Vẫn nằm tốt trong L2 Cache.
* **$B$ quá lớn ($B \ge 512$)**: Dung lượng vượt quá L2 Cache $\rightarrow$ Bị tràn cache và mất tác dụng tối ưu.
* **$B$ quá nhỏ ($B \le 8$)**: Tăng chi phí quản lý vòng lặp và không tận dụng hết băng thông nạp 64-byte của mỗi Cache Line.
* **Khối tối ưu**: **$B = 64 \times 64$** hoặc **$B = 128 \times 128$** là các kích thước khối chuẩn trên các hệ thống tính toán khoa học.
