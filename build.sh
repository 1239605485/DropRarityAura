#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TARGET="${1:-android-arm64}"
KERNELLOADER_DIR="${KERNELLOADER_DIR:-}"
if [[ -z "$KERNELLOADER_DIR" || ! -d "$KERNELLOADER_DIR/includes" ]]; then
  echo "Set KERNELLOADER_DIR to the KernelLoader source directory (headers required)." >&2
  exit 2
fi

mkdir -p "$ROOT/lib" "$ROOT/dist"
COMMON=("$ROOT/src/mod_entry.cpp" "$ROOT/src/item_draw_hook.cpp" "$ROOT/src/rarity_palette.cpp" "$ROOT/src/beam_renderer.cpp" "$ROOT/src/diagnostics.cpp" "$ROOT/src/kernel_api.cpp")
INCLUDES=(-I"$KERNELLOADER_DIR/includes" -I"$KERNELLOADER_DIR/mod-api" -I"$ROOT/src")
case "$TARGET" in
  android-arm64)
    : "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an installed Android NDK}"
    HOST_TAG="linux-x86_64"
    CXX="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/bin/aarch64-linux-android23-clang++"
    "$CXX" -std=c++17 -O2 -fPIC -shared -static-libstdc++ "${INCLUDES[@]}" "${COMMON[@]}" \
      -Wl,--no-undefined -o "$ROOT/lib/libdrop_rarity_aura.android.arm64.so"
    LIB="lib/libdrop_rarity_aura.android.arm64.so"
    ;;
  *) echo "Unsupported target: $TARGET" >&2; exit 2 ;;
esac

STAGE="$ROOT/dist/$TARGET"
rm -rf "$STAGE"
mkdir -p "$STAGE/Resources/lib"
cp "$ROOT/Info.json" "$ROOT/Manifest.json" "$ROOT/mod.json" "$STAGE/"
cp "$ROOT/$LIB" "$STAGE/Resources/lib/"
python3 - "$ROOT" "$STAGE" "$(dirname "$CXX")/llvm-readelf" <<'PY'
from pathlib import Path
import sys, zipfile, json, struct, subprocess, tempfile, shutil
root, stage = Path(sys.argv[1]), Path(sys.argv[2])
out = root / f"DropRarityAura-{stage.name}.zip"
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(stage.rglob("*")):
        if path.is_file():
            archive.write(path, path.relative_to(stage).as_posix())
# Model TEFManager: install the entry config separately and copy only Resources
# into the private directory, then resolve the path exactly as KernelLoader does.
with tempfile.TemporaryDirectory() as tmp:
    private = Path(tmp) / "private"
    shutil.copytree(stage / "Resources", private)
    config = json.loads((stage / "mod.json").read_text())
    binary = private / "lib" / f"lib{config['lib_name']}.android.arm64.so"
    data = binary.read_bytes()
    assert data[:4] == b"\x7fELF" and data[4] == 2
    assert struct.unpack_from("<H", data, 18)[0] == 183, "Not AArch64"
    symbols = subprocess.check_output([sys.argv[3], "-Ws", str(binary)], text=True)
    for symbol in ("patchlib_type_get_type", "patchlib_field_get_pointer", "patchlib_install_prepost_hook"):
        rows = [line.split() for line in symbols.splitlines() if line.split() and line.split()[-1] == symbol]
        assert rows and all(row[3] == "OBJECT" and row[6] != "UND" for row in rows), symbol
    deps = subprocess.check_output([sys.argv[3], "-d", str(binary)], text=True)
    assert "libc++_shared.so" not in deps, "Unbundled C++ runtime dependency"
print(f"Built and validated {out}")
PY
