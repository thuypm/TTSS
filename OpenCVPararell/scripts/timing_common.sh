#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

ensure_msys2_toolchain() {
    local msys_bin=""
    local candidate

    for candidate in \
        "/c/msys64/mingw64/bin" \
        "/mingw64/bin" \
        "C:/msys64/mingw64/bin"; do
        if [[ -x "$candidate/cmake.exe" && -x "$candidate/g++.exe" ]]; then
            msys_bin="$candidate"
            break
        fi
    done

    if [[ -n "$msys_bin" ]]; then
        export PATH="$msys_bin:$PATH"
        export MSYS2_BIN="$msys_bin"
        echo "Using MSYS2 toolchain: $msys_bin"
    fi
}

detect_opencv_dir() {
    local -n out_var=$1

    if [[ -n "${out_var:-}" ]]; then
        return
    fi

    local candidate
    for candidate in \
        "/c/msys64/mingw64/lib/cmake/opencv4" \
        "/mingw64/lib/cmake/opencv4" \
        "C:/msys64/mingw64/lib/cmake/opencv4" \
        "/c/msys64/mingw64/lib/cmake/OpenCV" \
        "/mingw64/lib/cmake/OpenCV"; do
        if [[ -f "$candidate/OpenCVConfig.cmake" ]]; then
            out_var="$candidate"
            echo "Auto-detected OpenCV_DIR: $out_var"
            return
        fi
    done
}

detect_generator_args() {
    if [[ -n "${CMAKE_GENERATOR:-}" ]]; then
        return
    fi

    case "${OSTYPE:-}" in
        msys*|mingw*|cygwin*)
            printf '%s\n' -G "MinGW Makefiles"
            ;;
    esac
}

configure_build() {
    local opencv_dir="${OpenCV_DIR:-}"

    ensure_msys2_toolchain
    detect_opencv_dir opencv_dir

    mkdir -p "$BUILD_DIR"

    echo "cmake: $(command -v cmake)"
    echo "g++:   $(command -v g++)"

    local cmake_args=(
        -S "$ROOT_DIR"
        -B "$BUILD_DIR"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )

    if [[ -n "${MSYS2_BIN:-}" ]]; then
        cmake_args+=(
            -DCMAKE_C_COMPILER="$MSYS2_BIN/gcc.exe"
            -DCMAKE_CXX_COMPILER="$MSYS2_BIN/g++.exe"
        )
    fi

    if [[ -n "$opencv_dir" ]]; then
        cmake_args+=(-DOpenCV_DIR="$opencv_dir")
    else
        echo "Warning: OpenCV_DIR not found. Try: export OpenCV_DIR=\"C:/msys64/mingw64/lib/cmake/opencv4\"" >&2
    fi

    mapfile -t generator_args < <(detect_generator_args)
    cmake "${cmake_args[@]}" "${generator_args[@]}"
    cmake --build "$BUILD_DIR"
}

app_path() {
    local app_name=$1

    if [[ -x "$BUILD_DIR/$app_name.exe" ]]; then
        printf '%s\n' "$BUILD_DIR/$app_name.exe"
    else
        printf '%s\n' "$BUILD_DIR/$app_name"
    fi
}

now_ms() {
    if date +%s%3N >/dev/null 2>&1; then
        date +%s%3N
    else
        echo $(( $(date +%s) * 1000 ))
    fi
}

parse_elapsed_ms() {
    local output=$1
    sed -n 's/^Processed [0-9][0-9]* image(s) in \([0-9.][0-9.]*\) ms\.$/\1/p' <<<"$output" | head -n 1
}

write_timing_file() {
    local timing_file=$1
    local program=$2
    local mode=$3
    local images=$4
    local elapsed_ms=$5
    local threads=$6
    local wall_clock_ms=$7
    local output_dir=$8

    mkdir -p "$(dirname "$timing_file")"

    cat >"$timing_file" <<EOF
program=$program
mode=$mode
images=$images
elapsed_ms=$elapsed_ms
threads=$threads
wall_clock_ms=$wall_clock_ms
output_dir=$output_dir
finished_at=$(date -Iseconds 2>/dev/null || date)
EOF

    echo "Timing saved to: $timing_file"
}
