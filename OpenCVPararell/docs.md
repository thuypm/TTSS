# PMT Scanner C++ OpenCV

Tài liệu này mô tả cách build, chạy và cấu trúc xử lý của project `OpenCVPararell`. Chương trình dùng C++17, OpenCV và OpenMP để xử lý ảnh phiếu trắc nghiệm PMT theo batch.

## Mục Tiêu

Chương trình nhận nhiều ảnh phiếu trong thư mục `input`, sau đó:

1. Đọc cấu hình vùng nhận diện từ `samples/paper-config.json`.
2. Tìm biên phiếu và crop/warp ảnh về đúng kích thước chuẩn.
3. Nhận diện các vùng tô gồm mã học sinh, mã đề, phần một, phần hai và phần ba.
4. Xuất ảnh đã căn chỉnh vào `output/warped`.
5. Xuất ảnh có đánh dấu đáp án đã nhận diện vào `output/annotated`.

Hiện tại chương trình tập trung vào pipeline xử lý ảnh và ảnh minh họa kết quả. Các file JSON/CSV kết quả chưa được ghi ra ở `main.cpp`.

## Yêu Cầu Môi Trường

- Windows + Git Bash hoặc MSYS2 shell.
- CMake 3.20 trở lên.
- Compiler hỗ trợ C++17, khuyến nghị MSYS2 MinGW64 `g++`.
- OpenCV cài qua MSYS2:

```bash
pacman -S mingw-w64-x86_64-opencv
```

OpenMP là tùy chọn nhưng được khuyến nghị. Nếu CMake tìm thấy OpenMP, chương trình sẽ tự bật macro `PMT_SCANNER_HAS_OPENMP` và chạy song song.

## Build Và Run Nhanh

Từ thư mục `OpenCVPararell`:

```bash
bash run.sh
```

Script sẽ tự:

- Ưu tiên toolchain tại `C:/msys64/mingw64/bin` nếu có.
- Tự dò `OpenCV_DIR` trong MSYS2.
- Configure CMake vào thư mục `build`.
- Build executable `pmt-scanner`.
- Chạy chương trình với input/output/config mặc định.

Các đường dẫn mặc định:

```text
input:  input
output: output
config: samples/paper-config.json
```

## Tùy Chọn Chạy

```bash
bash run.sh [options]
```

Các tùy chọn thường dùng:

```text
--input PATH        Thư mục ảnh đầu vào
--output PATH       Thư mục xuất kết quả
--config PATH       File cấu hình giấy thi
--threads auto|N    Số luồng xử lý, mặc định auto
--backend NAME      Backend xử lý, hiện tại dùng cpu
--build-only        Chỉ build, không chạy app
--clean             Xóa build cũ rồi configure lại
--watch             Tự rebuild/rerun khi source thay đổi
--opencv-dir PATH   Truyền OpenCV_DIR thủ công cho CMake
```

Ví dụ:

```bash
bash run.sh --threads auto
bash run.sh --threads 4
bash run.sh --input input --output output --config samples/paper-config.json
bash run.sh --build-only
```

Nếu cần truyền trực tiếp vào executable sau khi đã build:

```bash
./build/pmt-scanner.exe --input input --output output --config samples/paper-config.json --threads auto --backend cpu
```

## Cơ Chế Thread Runtime

Tham số `--threads` hỗ trợ hai chế độ:

- `--threads auto`: tự lấy số logical CPU tại runtime.
- `--threads N`: ép số luồng xử lý là `N`.

Khi build có OpenMP, `auto` dùng:

```cpp
omp_get_num_procs()
```

Nếu không có OpenMP, chương trình fallback sang:

```cpp
std::thread::hardware_concurrency()
```

Sau khi chạy, chương trình in số luồng thực tế:

```text
Threads: 12
```

## So Sánh Tuần Tự vs Song Song

Project có 2 chương trình riêng và 2 script chạy riêng:

| Chương trình | Script | Output ảnh | File thời gian |
|---|---|---|---|
| `pmt-scanner` (OpenMP) | `run_parallel.sh` | `output-parallel/` | `output/timing/parallel.txt` |
| `pmt-scanner-sequential` | `run_sequential.sh` | `output-sequential/` | `output/timing/sequential.txt` |

Chạy song song:

```bash
bash run_parallel.sh
bash run_sequential.sh
```

Ví dụ nội dung `output/timing/parallel.txt`:

```text
program=pmt-scanner
mode=parallel
images=12
elapsed_ms=386.23
threads=12
wall_clock_ms=402
output_dir=output-parallel
finished_at=2026-06-18T21:30:00+07:00
```

Ví dụ nội dung `output/timing/sequential.txt`:

```text
program=pmt-scanner-sequential
mode=sequential
images=12
elapsed_ms=4520.35
threads=1
wall_clock_ms=4535
output_dir=output-sequential
finished_at=2026-06-18T21:31:00+07:00
```

Tùy chọn script:

```text
--build-only        Chỉ build
--input PATH        Thư mục ảnh
--output PATH       Thư mục output riêng
--timing-file PATH  Đổi file ghi thời gian
--threads auto|N    Chỉ dùng cho run_parallel.sh
```

## Song Song Hóa Bằng OpenMP

Project có hai tầng song song:

1. Tầng batch ảnh trong `ImageProcessor.cpp`.
2. Tầng nhận diện ROI trong từng ảnh tại `RoiDetector.cpp`.

### Tầng Batch

`processImageBatch()` dùng OpenMP `parallel for` để xử lý nhiều ảnh cùng lúc:

```cpp
#pragma omp parallel for schedule(dynamic) num_threads(batchThreads) if(batchThreads > 1)
```

Số thread batch được giới hạn bởi:

- số thread runtime đã resolve từ `--threads`;
- số lượng ảnh thực tế trong input.

Ví dụ có 12 logical CPU nhưng chỉ có 4 ảnh thì batch chỉ cần tối đa 4 thread.

### Tầng ROI

`detectRois()` dùng OpenMP `parallel sections` cho 5 nhóm nhận diện độc lập:

- `studentId`
- `key`
- `partOne`
- `partTwo`
- `partThree`

Mỗi nhóm có thể chạy ở một section riêng:

```cpp
#pragma omp parallel sections num_threads(roiThreads) if(!omp_in_parallel() && roiThreads > 1)
```

Điều kiện `!omp_in_parallel()` giúp tránh tạo quá nhiều thread lồng nhau. Khi batch đang xử lý nhiều ảnh song song, ROI bên trong mỗi ảnh sẽ không spawn thêm vùng parallel mới. Khi chạy ít ảnh hoặc gọi `detectRois()` độc lập, ROI vẫn có thể chạy song song tối đa 5 section.

## Pipeline Xử Lý

Luồng xử lý chính nằm trong `main.cpp`:

1. Parse CLI options.
2. Load `paper-config.json`.
3. Tạo các thư mục output.
4. Scan danh sách ảnh đầu vào.
5. Resolve số thread runtime.
6. Gọi `processImageBatch()`.
7. In thời gian xử lý và số thread.

Trong mỗi ảnh, `processOneImage()` thực hiện:

1. Đọc ảnh bằng `cv::imread`.
2. Crop và warp ảnh bằng `cropAndWarp()`.
3. Nhận diện ROI bằng `detectRois()`.
4. Ghi ảnh warped vào `output/warped`.
5. Ghi ảnh annotated vào `output/annotated`.

## Cấu Trúc Source Chính

```text
OpenCVPararell/
├── CMakeLists.txt
├── run.sh
├── package.sh
├── include/
│   ├── Config.hpp
│   ├── CropAndWarp.hpp
│   ├── ImageFileScanner.hpp
│   ├── ImageProcessor.hpp
│   ├── RoiDetector.hpp
│   └── Types.hpp
├── src/
│   ├── main.cpp
│   ├── Config.cpp
│   ├── CropAndWarp.cpp
│   ├── ImageFileScanner.cpp
│   ├── ImageProcessor.cpp
│   ├── Annotator.cpp
│   ├── AnnotatedResultImage.cpp
│   ├── RoiDetector.cpp
│   └── RoiDetector/
│       ├── Common.cpp
│       ├── StudentIdKey.cpp
│       ├── PartOneTwo.cpp
│       └── PartThree.cpp
├── input/
├── output/
└── samples/
```

## Vai Trò Các Module

`Config.cpp` đọc và parse `paper-config.json`, sau đó map các vùng bubble vào `PaperConfig`.

`ImageFileScanner.cpp` quét ảnh đầu vào theo các định dạng phổ biến như JPG/PNG.

`CropAndWarp.cpp` tìm contour phiếu, xác định 4 góc và warp ảnh về kích thước chuẩn trong config.

`ImageProcessor.cpp` điều phối xử lý từng ảnh và xử lý batch bằng OpenMP.

`RoiDetector.cpp` là orchestrator nhận diện ROI, gọi các detector con.

`src/RoiDetector/Common.cpp` chứa các hàm xử lý ảnh dùng chung như tính độ tối, adaptive threshold và chọn đáp án thắng.

`src/RoiDetector/StudentIdKey.cpp` nhận diện mã học sinh và mã đề theo cột.

`src/RoiDetector/PartOneTwo.cpp` nhận diện phần một và phần hai.

`src/RoiDetector/PartThree.cpp` nhận diện phần ba.

`AnnotatedResultImage.cpp` vẽ các bubble đã nhận diện lên ảnh kết quả.

## Output

Sau khi chạy thành công:

```text
output/
├── warped/
│   └── demo_001.jpg ...
└── annotated/
    └── demo_001.jpg ...
```

`warped` là ảnh đã crop/căn phối cảnh. `annotated` là ảnh có đánh dấu vùng được nhận diện.

## Lưu Ý Hiện Tại

- `--backend cuda` đã có trong CLI nhưng chưa có xử lý CUDA thực tế.
- Crop/warp hiện dựa trên contour 4 góc rõ ràng, chưa có fallback marker đầy đủ.
- `ResultWriter.cpp` vẫn còn trong project nhưng `main.cpp` hiện không ghi JSON/CSV.
- Kết quả nhận diện đang được dùng để vẽ ảnh annotated và giữ trong bộ nhớ.

## Lỗi Thường Gặp

Nếu CMake không tìm thấy OpenCV, truyền `OpenCV_DIR` thủ công:

```bash
bash run.sh --opencv-dir "C:/msys64/mingw64/lib/cmake/opencv4"
```

Nếu build dùng nhầm compiler khác MSYS2, kiểm tra log của `run.sh`:

```text
cmake: ...
g++:   ...
```

Nên thấy `g++` trỏ về `C:/msys64/mingw64/bin/g++.exe`.

Nếu output không có ảnh annotated, kiểm tra:

- ảnh input đọc được không;
- config có đúng kích thước và tọa độ không;
- ảnh có đủ 4 cạnh/góc rõ để crop/warp không.

