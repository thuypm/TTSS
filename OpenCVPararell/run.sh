#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
APP_NAME="pmt-scanner"

INPUT_DIR="$ROOT_DIR/input"
OUTPUT_DIR="$ROOT_DIR/output"
CONFIG_FILE="$ROOT_DIR/samples/paper-config.json"
THREADS="auto"
BACKEND="cpu"
WATCH="0"
CLEAN="0"
BUILD_ONLY="0"
OPENCV_DIR_VALUE="${OpenCV_DIR:-}"
EXTRA_APP_ARGS=()

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
    local candidate

    if [[ -n "$OPENCV_DIR_VALUE" ]]; then
        return
    fi

    for candidate in \
        "/c/msys64/mingw64/lib/cmake/opencv4" \
        "/mingw64/lib/cmake/opencv4" \
        "C:/msys64/mingw64/lib/cmake/opencv4" \
        "/c/msys64/mingw64/lib/cmake/OpenCV" \
        "/mingw64/lib/cmake/OpenCV"; do
        if [[ -f "$candidate/OpenCVConfig.cmake" ]]; then
            OPENCV_DIR_VALUE="$candidate"
            echo "Auto-detected OpenCV_DIR: $OPENCV_DIR_VALUE"
            return
        fi
    done
}

print_usage() {
    cat <<EOF
Usage: ./run.sh [options] [-- extra app args]

Options:
  --watch                 Rebuild and rerun when src/include files change
  --clean                 Remove build directory before configuring
  --build-only            Configure and build, do not run the app
  --opencv-dir PATH       OpenCV_DIR path passed to CMake
  --input PATH            Input image directory (default: input)
  --output PATH           Output directory (default: output)
  --config PATH           Paper config JSON (default: samples/paper-config.json)
  --threads auto|N        Number of processing threads (default: auto)
  --backend NAME          Backend name (default: cpu)
  -h, --help              Show this help

Examples:
  ./run.sh
  ./run.sh --opencv-dir "D:/opencv/build"
  ./run.sh --watch --threads 8
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --watch)
            WATCH="1"
            shift
            ;;
        --clean)
            CLEAN="1"
            shift
            ;;
        --build-only)
            BUILD_ONLY="1"
            shift
            ;;
        --opencv-dir)
            OPENCV_DIR_VALUE="${2:?Missing value for --opencv-dir}"
            shift 2
            ;;
        --input)
            INPUT_DIR="${2:?Missing value for --input}"
            shift 2
            ;;
        --output)
            OUTPUT_DIR="${2:?Missing value for --output}"
            shift 2
            ;;
        --config)
            CONFIG_FILE="${2:?Missing value for --config}"
            shift 2
            ;;
        --threads)
            THREADS="${2:?Missing value for --threads}"
            shift 2
            ;;
        --backend)
            BACKEND="${2:?Missing value for --backend}"
            shift 2
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        --)
            shift
            EXTRA_APP_ARGS+=("$@")
            break
            ;;
        *)
            echo "Unknown option: $1" >&2
            print_usage
            exit 1
            ;;
    esac
done

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

app_path() {
    if [[ -x "$BUILD_DIR/$APP_NAME.exe" ]]; then
        printf '%s\n' "$BUILD_DIR/$APP_NAME.exe"
    else
        printf '%s\n' "$BUILD_DIR/$APP_NAME"
    fi
}

configure_build() {
    ensure_msys2_toolchain
    detect_opencv_dir

    if [[ "$CLEAN" == "1" ]]; then
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$OUTPUT_DIR"

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

    if [[ -n "$OPENCV_DIR_VALUE" ]]; then
        cmake_args+=(-DOpenCV_DIR="$OPENCV_DIR_VALUE")
    else
        echo "Warning: OpenCV_DIR not found. Try: ./run.sh --opencv-dir \"C:/msys64/mingw64/lib/cmake/opencv4\"" >&2
    fi

    mapfile -t generator_args < <(detect_generator_args)
    cmake "${cmake_args[@]}" "${generator_args[@]}"
    cmake --build "$BUILD_DIR"
}

run_app() {
    local executable
    executable="$(app_path)"

    "$executable" \
        --input "$INPUT_DIR" \
        --output "$OUTPUT_DIR" \
        --config "$CONFIG_FILE" \
        --threads "$THREADS" \
        --backend "$BACKEND" \
        "${EXTRA_APP_ARGS[@]}"
}

source_signature() {
    (
        cd "$ROOT_DIR"
        {
            printf '%s\n' "CMakeLists.txt"
            find src include samples -type f \
                \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.json' \) \
                -print 2>/dev/null
        } | sort | xargs cksum 2>/dev/null || true
    )
}

build_and_run() {
    configure_build

    if [[ "$BUILD_ONLY" == "0" ]]; then
        run_app
    fi
}

build_and_run

if [[ "$WATCH" == "1" ]]; then
    echo "Watching src/include/samples... Press Ctrl+C to stop."
    last_signature="$(source_signature)"

    while true; do
        sleep 1
        current_signature="$(source_signature)"

        if [[ "$current_signature" != "$last_signature" ]]; then
            echo
            echo "Change detected. Rebuilding..."
            last_signature="$current_signature"

            if ! build_and_run; then
                echo "Build or run failed. Waiting for next change..." >&2
            fi
        fi
    done
fi
