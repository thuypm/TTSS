#!/bin/bash
# run_benchmark.sh - Build và chạy benchmark
set -e

echo "================================================"
echo "  Linear Solver Benchmark: Sequential vs OpenMP"
echo "================================================"
echo ""

# Build
echo "[1/3] Building..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3" > /dev/null 2>&1
make -j$(nproc) 2>&1 | grep -E "(Error|Warning|Built|make)" || true
cd ..
echo "      Build complete."
echo ""

# Run Sequential
echo "[2/3] Running Sequential..."
./build/sequential
echo ""

# Run Parallel
echo "[3/3] Running Parallel (OpenMP)..."
./build/parallel
echo ""

# Merge & compute speedup
echo "[Done] Results saved:"
echo "  - results_sequential.csv"
echo "  - results_parallel.csv"
echo ""
echo "Mở dashboard.html trong trình duyệt để xem biểu đồ so sánh."
