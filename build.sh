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
COMMON=("$ROOT/src/mod_entry.cpp" "$ROOT/src/rarity_palette.cpp" "$ROOT/src/diagnostics.cpp" "$ROOT/src/kernel_api.cpp" "$ROOT/src/loot_style.cpp" "$ROOT/src/rarity_profile.cpp")
INCLUDES=(-I"$KERNELLOADER_DIR/includes" -I"$KERNELLOADER_DIR/mod-api" -I"$ROOT/src")
FLAGS=(-std=c++17 -O2 -shared -static-libstdc++ -static-libgcc -Wl,--no-undefined)
case "$TARGET" in
  android-arm64)
    : "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an installed Android NDK}"
    CXX="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android23-clang++"
    FLAGS+=(-fPIC)
    COMMON+=("$ROOT/src/item_draw_hook.cpp" "$ROOT/src/beam_renderer.cpp")
    LIB="lib/libdrop_rarity_aura.android.arm64.so"
    INSPECT="$(dirname "$CXX")/llvm-readelf"
    ;;
  windows-x86|windows-x64)
    if [[ "$TARGET" == windows-x86 ]]; then TRIPLE=i686-w64-mingw32; ARCH=x86; else TRIPLE=x86_64-w64-mingw32; ARCH=x64; fi
    CXX="${TRIPLE}-g++-posix"
    FLAGS+=(-static -Wl,--export-all-symbols)
    COMMON+=("$ROOT/src/desktop_item_draw_hook.cpp" "$ROOT/src/desktop_beam_renderer.cpp")
    LIB="lib/drop_rarity_aura.windows.$ARCH.dll"
    INSPECT="${TRIPLE}-objdump"
    ;;
  linux-x86|linux-x64)
    CXX=g++
    if [[ "$TARGET" == linux-x86 ]]; then FLAGS+=(-m32); ARCH=x86; else FLAGS+=(-m64); ARCH=x64; fi
    FLAGS+=(-fPIC)
    COMMON+=("$ROOT/src/desktop_item_draw_hook.cpp" "$ROOT/src/desktop_beam_renderer.cpp")
    LIB="lib/libdrop_rarity_aura.linux.$ARCH.so"
    INSPECT=readelf
    ;;
  *) echo "Unsupported target: $TARGET" >&2; exit 2 ;;
esac
"$CXX" "${FLAGS[@]}" "${INCLUDES[@]}" "${COMMON[@]}" -o "$ROOT/$LIB"

STAGE="$ROOT/dist/$TARGET"
rm -rf "$STAGE"
mkdir -p "$STAGE/Resources/lib"
cp "$ROOT/Info.json" "$ROOT/Manifest.json" "$ROOT/mod.json" "$STAGE/"
cp "$ROOT/$LIB" "$STAGE/Resources/lib/"
python3 "$ROOT/scripts/generate_effects.py" "$STAGE/Resources/effects"
python3 - "$ROOT" "$STAGE" "$INSPECT" "$(basename "$LIB")" <<'PY'
from pathlib import Path
import sys, zipfile, json, struct, subprocess, tempfile, shutil, re
root, stage = Path(sys.argv[1]), Path(sys.argv[2])
info = json.loads((stage / "Info.json").read_text())
version_header = (root / "src/version.hpp").read_text()
assert re.search(r'kModVersion = "' + re.escape(info['version']) + r'"', version_header)
assert re.search(r'kModVersionCode = ' + str(info['versionCode']) + r';', version_header)
expected_files = {"Info.json", "Manifest.json", "mod.json", "Resources/lib/" + sys.argv[4]}
expected_files |= {"Resources/effects/" + n + ".rgba" for n in ("bloom", "ring", "star", "spark", "beam", "tail")}
assert {p.relative_to(stage).as_posix() for p in stage.rglob("*") if p.is_file()} == expected_files, "Unexpected or obsolete package files"
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
    binary = private / "lib" / sys.argv[4]
    for name in ("bloom", "ring", "star", "spark", "beam", "tail"):
        expected = (96 if name in ("beam", "tail") else 192) * 192 * 4
        assert (private / "effects" / (name + ".rgba")).stat().st_size == expected
    data = binary.read_bytes()
    target = stage.name
    if target.startswith("windows-"):
        assert data[:2] == b"MZ"
        pe = struct.unpack_from("<I", data, 0x3c)[0]
        assert data[pe:pe+4] == b"PE\0\0"
        assert struct.unpack_from("<H", data, pe+4)[0] == (0x14c if target.endswith("x86") else 0x8664)
        exports = subprocess.check_output([sys.argv[3], "-p", str(binary)], text=True)
        for symbol in ("create_kernel_mod", "patchlib_type_get_type", "patchlib_field_get_value", "patchlib_install_prepost_hook"):
            assert re.search(r"\b" + symbol + r"\b", exports), symbol
        for dependency in ("libstdc++", "libgcc_s", "libwinpthread"):
            assert dependency not in exports, "Unbundled C++ runtime dependency"
    else:
        assert data[:4] == b"\x7fELF"
        elf_class = 1 if target.endswith("x86") else 2
        assert data[4] == elf_class
        expected_machine = 183 if target.startswith("android-") else (3 if elf_class == 1 else 62)
        assert struct.unpack_from("<H", data, 18)[0] == expected_machine
        symbols = subprocess.check_output([sys.argv[3], "-Ws", str(binary)], text=True)
        for symbol in ("create_kernel_mod", "patchlib_type_get_type", "patchlib_field_get_pointer" if target.startswith("android-") else "patchlib_field_get_value", "patchlib_install_prepost_hook"):
            rows = [line.split() for line in symbols.splitlines() if line.split() and line.split()[-1] == symbol]
            assert rows and all(row[6] != "UND" for row in rows), symbol
            if symbol != "create_kernel_mod": assert all(row[3] == "OBJECT" for row in rows), symbol
        deps = subprocess.check_output([sys.argv[3], "-d", str(binary)], text=True)
        assert "libc++_shared.so" not in deps and "libstdc++.so" not in deps

print(f"Built and validated {out}")
PY
