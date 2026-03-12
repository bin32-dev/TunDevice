#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [[ "${1:-}" == "--clean" ]]; then
  rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel

echo "Build complete."
echo "  Binary: ${BUILD_DIR}/bin/tun_proxy_ui"
echo "  Library: ${BUILD_DIR}/lib/libtunlib.so"
