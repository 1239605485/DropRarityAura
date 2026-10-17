#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
out="droprarityaura.tefpkg"
rm -f "$out"
zip -qr "$out" Info.json Manifest.json luamod.json Resources
printf 'Built %s\n' "$out"
