# PMT Scanner C++

CLI C++/OpenCV để đọc ảnh từ `input/`, nhận diện phiếu trắc nghiệm và xuất kết quả vào `output/`.

Project này bám theo tài liệu migration:

- đọc danh sách ảnh `.jpg`, `.jpeg`, `.png`;
- xử lý từng ảnh bằng OpenCV native;
- tách module `CropAndWarp`, `RoiDetector`, `ImageProcessor`, `ResultWriter`;
- tổ chức theo hướng lập trình cấu trúc: dữ liệu nằm trong `struct`, xử lý nằm trong hàm tự do;
- xử lý batch ảnh bằng OpenMP khi compiler hỗ trợ;
- xuất `results.json`, `results.csv`, ảnh `warped/`, `annotated/` và log lỗi.

## Cấu Trúc

```text
.
├── CMakeLists.txt
├── include/
├── src/
├── input/
├── output/
│   ├── annotated/
│   ├── logs/
│   └── warped/
└── samples/
    └── paper-config.json
```

## Build

Máy cần có:

- C++ compiler, ví dụ MinGW-W64 `g++`;
- CMake;
- OpenCV C++ đã cài và được CMake tìm thấy.

Ví dụ build:

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Nếu OpenCV không nằm trong PATH/CMake prefix, truyền thêm `OpenCV_DIR`:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DOpenCV_DIR="D:/opencv/build"
```

## Chạy

```bash
./build/pmt-scanner.exe --input input --output output --config samples/paper-config.json --threads 4 --backend cpu
```

Giai đoạn hiện tại là skeleton: đã scan ảnh, đọc ảnh bằng OpenCV, tạo output JSON/CSV và lưu ảnh warped/annotated dạng passthrough. Logic crop/warp và ROI detector sẽ được port dần từ bản Electron/OpenCV.js.

## Hướng Thiết Kế

Project hạn chế dùng class/OOP. Các module hiện tại nên giữ theo nguyên tắc:

- `Types.hpp` chứa các `struct` dữ liệu dùng chung.
- Mỗi file trong `src/` chứa nhóm hàm xử lý một bước cụ thể.
- `ImageProcessJob` là đơn vị công việc độc lập cho từng ảnh.
- `processImageBatch()` là điểm gom vòng lặp song song, giúp dễ đổi chiến lược OpenMP/CUDA/TBB sau này mà không làm phình `main`.
