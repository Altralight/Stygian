#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

TARGET="${1:-}"
GROUP="${2:-}"
CLANG_BIN="${CLANG:-clang}"
MANIFEST="$ROOT/compile/targets.json"

if [[ ! -f "$MANIFEST" ]]; then
  echo "missing manifest: $MANIFEST" >&2
  exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
  echo "jq is required for compile/macos/build.sh" >&2
  exit 1
fi

mkdir -p build

build_one() {
  local name="$1"
  local backend entry stem
  local zstd_prefix=""
  backend="$(jq -r --arg n "$name" '.targets[$n].backend // empty' "$MANIFEST")"
  entry="$(jq -r --arg n "$name" '.targets[$n].entry_source // empty' "$MANIFEST")"
  stem="$(jq -r --arg n "$name" '.targets[$n].output_stem // empty' "$MANIFEST")"
  if [[ -z "$backend" || -z "$entry" || -z "$stem" ]]; then
    echo "unknown target: $name" >&2
    exit 1
  fi

  common_flags=()
  while IFS= read -r line; do
    common_flags+=("$line")
  done < <(jq -r '.common.flags[]' "$MANIFEST")
  common_includes=()
  while IFS= read -r line; do
    common_includes+=("$line")
  done < <(jq -r '.common.includes[]' "$MANIFEST")
  common_sources=()
  while IFS= read -r line; do
    common_sources+=("$line")
  done < <(jq -r '.common.sources[]' "$MANIFEST")
  common_defines=()
  while IFS= read -r line; do
    common_defines+=("$line")
  done < <(jq -r '.common.defines[]' "$MANIFEST")

  args=()
  args+=("${common_flags[@]}")
  args+=("-D_DARWIN_C_SOURCE" "-D_POSIX_C_SOURCE=200809L"
         "-DGL_SILENCE_DEPRECATION")
  if command -v brew >/dev/null 2>&1; then
    zstd_prefix="$(brew --prefix zstd 2>/dev/null || true)"
    if [[ -n "$zstd_prefix" ]]; then
      args+=("-I" "$zstd_prefix/include" "-L" "$zstd_prefix/lib")
    fi
  fi
  for inc in "${common_includes[@]}"; do
    args+=("-I" "$inc")
  done
  for def in "${common_defines[@]}"; do
    args+=("-D${def}")
  done

  args+=("$entry")
  args+=("${common_sources[@]}")
  args+=("window/platform/stygian_cocoa.m" "window/platform/stygian_cocoa_shim.m")

  if [[ "$backend" == "vk" ]]; then
    args+=("$(jq -r '.common.vk_backend_source' "$MANIFEST")")
    args+=("-DSTYGIAN_DEMO_VULKAN" "-DSTYGIAN_VULKAN")
    args+=("-lvulkan")
  else
    args+=("$(jq -r '.common.gl_backend_source' "$MANIFEST")")
    args+=("-framework" "OpenGL")
  fi

  args+=("-o" "build/${stem}")
  args+=("-framework" "Cocoa" "-framework" "QuartzCore" "-framework" "IOKit" "-framework" "CoreVideo")
  args+=("-lz" "-lzstd")

  echo "[${name}] Building..."
  "$CLANG_BIN" "${args[@]}"
  echo "[${name}] Build SUCCESS: build/${stem}"
}

if [[ -n "$GROUP" ]]; then
  group_targets=()
  while IFS= read -r line; do
    group_targets+=("$line")
  done < <(jq -r --arg g "$GROUP" '.groups[$g][]? // empty' "$MANIFEST")
  if [[ ${#group_targets[@]} -eq 0 ]]; then
    echo "unknown group: $GROUP" >&2
    exit 1
  fi
  for t in "${group_targets[@]}"; do
    build_one "$t"
  done
  echo "[${GROUP}] Build SUCCESS"
  exit 0
fi

if [[ -z "$TARGET" ]]; then
  echo "usage: compile/macos/build.sh <target> [group]" >&2
  exit 1
fi

build_one "$TARGET"
