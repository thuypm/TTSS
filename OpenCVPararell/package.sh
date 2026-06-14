#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
DIST_DIR="$ROOT_DIR/dist"
APP_NAME="pmt-scanner"
ZIP_NAME="pmt-scanner-win64.zip"
OPENCV_DIR_VALUE="${OpenCV_DIR:-}"
MAKE_ZIP="1"
CLEAN_BUILD="0"

print_usage() {
    cat <<EOF
Usage: ./package.sh [options]

Build pmt-scanner and create a portable dist/ folder with required DLLs.

Options:
  --no-zip                Do not create zip archive
  --clean                 Clean build directory before building
  --opencv-dir PATH       OpenCV_DIR path passed to CMake
  -h, --help              Show this help

Output:
  dist/
    pmt-scanner.exe
    *.dll
    samples/
    input/
    output/
    run.bat
    README.txt
  ${ZIP_NAME}
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-zip)
            MAKE_ZIP="0"
            shift
            ;;
        --clean)
            CLEAN_BUILD="1"
            shift
            ;;
        --opencv-dir)
            OPENCV_DIR_VALUE="${2:?Missing value for --opencv-dir}"
            shift 2
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            print_usage
            exit 1
            ;;
    esac
done

ensure_msys2_toolchain() {
    local msys_bin=""
    local candidate

    for candidate in \
        "/c/msys64/mingw64/bin" \
        "/mingw64/bin" \
        "C:/msys64/mingw64/bin"; do
        if [[ -x "$candidate/cmake.exe" && -x "$candidate/g++.exe" && -x "$candidate/objdump.exe" ]]; then
            msys_bin="$candidate"
            break
        fi
    done

    if [[ -z "$msys_bin" ]]; then
        echo "MSYS2 MinGW64 toolchain not found. Install packages:" >&2
        echo "  pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-opencv" >&2
        exit 1
    fi

    export PATH="$msys_bin:$PATH"
    export MSYS2_BIN="$msys_bin"
    echo "Using MSYS2 toolchain: $msys_bin"
}

detect_opencv_dir() {
    local candidate

    if [[ -n "$OPENCV_DIR_VALUE" ]]; then
        return
    fi

    for candidate in \
        "/c/msys64/mingw64/lib/cmake/opencv4" \
        "/mingw64/lib/cmake/opencv4" \
        "C:/msys64/mingw64/lib/cmake/opencv4"; do
        if [[ -f "$candidate/OpenCVConfig.cmake" ]]; then
            OPENCV_DIR_VALUE="$candidate"
            echo "Auto-detected OpenCV_DIR: $OPENCV_DIR_VALUE"
            return
        fi
    done

    echo "OpenCV_DIR not found. Use: ./package.sh --opencv-dir \"C:/msys64/mingw64/lib/cmake/opencv4\"" >&2
    exit 1
}

is_system_dll() {
    local name
    name="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"

    case "$name" in
        kernel32.dll|msvcrt.dll|ole32.dll|opengl32.dll|user32.dll|advapi32.dll|\
        shell32.dll|ws2_32.dll|gdi32.dll|winmm.dll|imm32.dll|comdlg32.dll|\
        shlwapi.dll|rpcrt4.dll|sechost.dll|bcrypt.dll|ntdll.dll|ucrtbase.dll)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

resolve_dll_path() {
    local dll_name="$1"
    local search_dir

    for search_dir in "$MSYS2_BIN" "$MSYS2_BIN/../lib"; do
        if [[ -f "$search_dir/$dll_name" ]]; then
            printf '%s\n' "$search_dir/$dll_name"
            return 0
        fi
    done

    return 1
}

declare -A COPIED_DLLS=()
declare -A SCANNED_BINARIES=()

copy_dll() {
    local dll_name="$1"
    local source_path

    if is_system_dll "$dll_name"; then
        return 0
    fi

    if [[ -n "${COPIED_DLLS[$dll_name]:-}" ]]; then
        return 0
    fi

    if ! source_path="$(resolve_dll_path "$dll_name")"; then
        echo "Warning: missing dependency $dll_name" >&2
        return 0
    fi

    cp -f "$source_path" "$DIST_DIR/"
    COPIED_DLLS["$dll_name"]=1
    echo "  + $dll_name"
}

collect_dlls_from_binary() {
    local binary_path="$1"
    local dll_name
    local -a pending=()

    if [[ -n "${SCANNED_BINARIES[$binary_path]:-}" ]]; then
        return 0
    fi
    SCANNED_BINARIES["$binary_path"]=1

    while IFS= read -r dll_name; do
        pending+=("$dll_name")
    done < <(
        objdump -p "$binary_path" \
            | awk '/DLL Name:/ { print $3 }' \
            | sort -u
    )

    for dll_name in "${pending[@]}"; do
        copy_dll "$dll_name"
    done

    for dll_name in "${pending[@]}"; do
        if [[ -f "$DIST_DIR/$dll_name" ]]; then
            collect_dlls_from_binary "$DIST_DIR/$dll_name"
        fi
    done
}

configure_build() {
    if [[ "$CLEAN_BUILD" == "1" ]]; then
        rm -rf "$BUILD_DIR"
    fi

    local cmake_args=(
        -S "$ROOT_DIR"
        -B "$BUILD_DIR"
        -G "MinGW Makefiles"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        -DCMAKE_CXX_COMPILER="$MSYS2_BIN/g++.exe"
    )

    cmake_args+=(-DOpenCV_DIR="$OPENCV_DIR_VALUE")
    cmake "${cmake_args[@]}"
    cmake --build "$BUILD_DIR" --config Release
}

app_path() {
    if [[ -f "$BUILD_DIR/$APP_NAME.exe" ]]; then
        printf '%s\n' "$BUILD_DIR/$APP_NAME.exe"
    else
        printf '%s\n' "$BUILD_DIR/$APP_NAME"
    fi
}

create_dist_layout() {
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR/input" "$DIST_DIR/output/annotated" "$DIST_DIR/output/warped" "$DIST_DIR/output/logs"
    cp -r "$ROOT_DIR/samples" "$DIST_DIR/"

    cat > "$DIST_DIR/run.bat" <<'EOF'
@echo off
setlocal
cd /d "%~dp0"
pmt-scanner.exe --input input --output output --config samples/paper-config.json --threads 4 --backend cpu %*
EOF

    cat > "$DIST_DIR/README.txt" <<'EOF'
PMT Scanner - Portable Windows build

Requirements:
- Windows 64-bit
- No need to install OpenCV or MSYS2

Run:
1. Put exam images into input/
2. Double-click run.bat
   or run from cmd:
   pmt-scanner.exe --input input --output output --config samples/paper-config.json --threads 4

Output:
- output/results.json
- output/results.csv
- output/warped/
- output/annotated/
- output/logs/
EOF
}

package_release() {
    local executable
    executable="$(app_path)"

    if [[ ! -f "$executable" ]]; then
        echo "Executable not found: $executable" >&2
        exit 1
    fi

    create_dist_layout
    cp -f "$executable" "$DIST_DIR/$APP_NAME.exe"

    echo "Collecting runtime DLLs..."
    collect_dlls_from_binary "$DIST_DIR/$APP_NAME.exe"

    echo
    echo "Package ready: $DIST_DIR"
    echo "Files:"
    ls -1 "$DIST_DIR"

    if [[ "$MAKE_ZIP" == "1" ]]; then
        (
            cd "$ROOT_DIR"
            rm -f "$ZIP_NAME"
            if command -v zip >/dev/null 2>&1; then
                zip -r "$ZIP_NAME" dist >/dev/null
            elif command -v 7z >/dev/null 2>&1; then
                7z a "$ZIP_NAME" dist >/dev/null
            else
                echo "zip/7z not found. Skipping archive creation." >&2
                return 0
            fi
        )
        echo "Archive: $ROOT_DIR/$ZIP_NAME"
    fi
}

ensure_msys2_toolchain
detect_opencv_dir
configure_build
package_release
