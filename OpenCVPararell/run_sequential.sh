#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/timing_common.sh
source "$ROOT_DIR/scripts/timing_common.sh"

INPUT_DIR="$ROOT_DIR/input"
OUTPUT_DIR="$ROOT_DIR/output-sequential"
CONFIG_FILE="$ROOT_DIR/samples/paper-config.json"
TIMING_FILE="$ROOT_DIR/output/timing/sequential.txt"
BUILD_ONLY="0"

print_usage() {
    cat <<EOF
Usage: ./run_sequential.sh [options]

Options:
  --build-only            Chỉ build, không chạy
  --input PATH            Thư mục ảnh đầu vào (default: input)
  --output PATH           Thư mục output (default: output-sequential)
  --config PATH           File config (default: samples/paper-config.json)
  --timing-file PATH      File ghi thời gian (default: output/timing/sequential.txt)
  -h, --help              Hiện trợ giúp
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-only)
            BUILD_ONLY="1"
            shift
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
        --timing-file)
            TIMING_FILE="${2:?Missing value for --timing-file}"
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

configure_build

if [[ "$BUILD_ONLY" == "1" ]]; then
    exit 0
fi

mkdir -p "$OUTPUT_DIR"
EXECUTABLE="$(app_path pmt-scanner-sequential)"

WALL_START="$(now_ms)"
APP_OUTPUT="$("$EXECUTABLE" \
    --input "$INPUT_DIR" \
    --output "$OUTPUT_DIR" \
    --config "$CONFIG_FILE")"
WALL_END="$(now_ms)"
WALL_CLOCK_MS=$((WALL_END - WALL_START))

echo "$APP_OUTPUT"

ELAPSED_MS="$(parse_elapsed_ms "$APP_OUTPUT")"
IMAGE_COUNT="$(sed -n 's/^Processed \([0-9][0-9]*\) image(s) in .*/\1/p' <<<"$APP_OUTPUT" | head -n 1)"

if [[ -z "$ELAPSED_MS" ]]; then
    ELAPSED_MS="$WALL_CLOCK_MS"
fi

if [[ -z "$IMAGE_COUNT" ]]; then
    IMAGE_COUNT="0"
fi

write_timing_file \
    "$TIMING_FILE" \
    "pmt-scanner-sequential" \
    "sequential" \
    "$IMAGE_COUNT" \
    "$ELAPSED_MS" \
    "1" \
    "$WALL_CLOCK_MS" \
    "$OUTPUT_DIR"
