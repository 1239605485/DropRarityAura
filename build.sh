#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TARGET="${1:-linux-x64}"
KERNELLOADER_DIR="${KERNELLOADER_DIR:-}"
if [[ -z "$KERNELLOADER_DIR" || ! -d "$KERNELLOADER_DIR/includes" ]]; then
  echo "Set KERNELLOADER_DIR to the KernelLoader source directory (headers required)." >&2
  exit 2
fi

mkdir -p "$ROOT/lib" "$ROOT/dist"
COMMON=("$ROOT/src/mod_entry.cpp" "$ROOT/src/item_draw_hook.cpp" "$ROOT/src/rarity_palette.cpp")
INCLUDES=(-I"$KERNELLOADER_DIR/includes" -I"$KERNELLOADER_DIR/mod-api" -I"$ROOT/src")
case "$TARGET" in
  linux-x64)
    c++ -std=c++17 -O2 -fPIC -shared "${INCLUDES[@]}" "${COMMON[@]}" \
      -Wl,--allow-shlib-undefined -o "$ROOT/lib/libdrop_rarity_aura.linux.x64.so"
    LIB="lib/libdrop_rarity_aura.linux.x64.so"
    ;;
  android-arm64)
    : "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an installed Android NDK}"
    HOST_TAG="linux-x86_64"
    CXX="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/bin/aarch64-linux-android23-clang++"
    "$CXX" -std=c++17 -O2 -fPIC -shared "${INCLUDES[@]}" "${COMMON[@]}" \
      -Wl,--allow-shlib-undefined -o "$ROOT/lib/libdrop_rarity_aura.android.arm64.so"
    LIB="lib/libdrop_rarity_aura.android.arm64.so"
    ;;
  *) echo "Unsupported target: $TARGET" >&2; exit 2 ;;
esac

STAGE="$ROOT/dist/$TARGET"
mkdir -p "$STAGE/lib"
cp "$ROOT/Info.json" "$ROOT/Manifest.json" "$ROOT/mod.json" "$STAGE/"
cp "$ROOT/$LIB" "$STAGE/lib/"
python3 - "$ROOT" "$STAGE" <<'PY'
from pathlib import Path
import sys, zipfile
root, stage = Path(sys.argv[1]), Path(sys.argv[2])
out = root / f"DropRarityAura-{stage.name}.zip"
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(stage.rglob("*")):
        if path.is_file():
            archive.write(path, path.relative_to(stage).as_posix())
print(f"Built {out}")
PY
