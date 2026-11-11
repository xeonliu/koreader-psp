#!/usr/bin/env bash
# Assemble a runnable KOReader 2018.12 directory for the PSP (or for PPSSPP's --root).
#   platform/psp/stage.sh STAGE_DIR /path/to/2018/koreader-base /path/to/prebuilt-common [EBOOT.PBP]
# prebuilt-common: a directory with the pure-Lua libraries (dkjson, ltn12, socket, ...), e.g. build/psp/common of
# the modern koreader-base PSP build.
set -euo pipefail
stage="$1"; base="$2"; common="$3"; eboot="${4:-$(dirname "$0")/launcher/EBOOT.PBP}"
here="$(cd "$(dirname "$0")" && pwd)"; ko="$(cd "$here/../.." && pwd)"
mkdir -p "$stage"
rm -rf "$stage/frontend" "$stage/plugins" "$stage/ffi" "$stage/common" "$stage/modules" "$stage/resources"
cp "$ko/reader.lua" "$ko/setupkoenv.lua" "$ko/defaults.lua" "$ko/datastorage.lua" "$stage/"
cp -r "$ko/frontend" "$ko/plugins" "$ko/resources" "$stage/"
# Fonts and sample books are not part of this tree: point KO_PSP_FONTS / KO_PSP_SAMPLES at directories that have them.
rm -rf "$stage/fonts" "$stage/resources/fonts"; cp -rL "${KO_PSP_FONTS:-$ko/resources/fonts}" "$stage/fonts"
cp -r "$base/ffi" "$stage/ffi"
cp -rL "$common" "$stage/common"
cp -r "$here/luastubs/." "$stage/common/" 2>/dev/null || true
mkdir -p "$stage/modules" "$stage/data" "$stage/libs" "$stage/settings" "$stage/cache" "$stage/clipboard" "$stage/history" "$stage/samples"
cp -f "$base"/build/psp/modules/*.prx "$stage/modules/" 2>/dev/null || true
cp -rf "$base"/build/psp/data/. "$stage/data/" 2>/dev/null || true
for f in juliet.epub leaves.epub sample.pdf 2col.pdf djvu3spec.djvu sample.txt; do cp -f "${KO_PSP_SAMPLES:-$ko/test}/$f" "$stage/samples/" 2>/dev/null || true; done
echo "${KO_PSP_VERSION:-v2018.12-psp}" > "$stage/git-rev"
cp -f "$eboot" "$stage/EBOOT.PBP"
echo "staged in $stage"
