#!/usr/bin/env bash
# Assemble a runnable KOReader directory for the PSP (or for PPSSPP's --root): the EBOOT plus KOReader's Lua tree.
#   platform/psp/stage.sh STAGE_DIR /path/to/koreader-base [EBOOT.PBP]
set -euo pipefail
stage="$1"; base="$2"; eboot="${3:-$(dirname "$0")/launcher/EBOOT.PBP}"
ko="$(cd "$(dirname "$0")/../.." && pwd)"
mkdir -p "$stage"
link() { rm -rf "$stage/$2"; ln -s "$1" "$stage/$2"; }
for f in reader.lua setupkoenv.lua defaults.lua datastorage.lua; do link "$ko/$f" "$f"; done
for d in frontend plugins resources; do link "$ko/$d" "$d"; done
link "$ko/resources/fonts" fonts
link "$base/ffi" ffi
# common/: the Lua libraries koreader-base builds, plus LuaSocket's pure-Lua modules and PSP stand-ins for its C cores.
rm -rf "$stage/common"; mkdir -p "$stage/common/socket" "$stage/common/mime"
for f in "$base"/build/psp/common/*; do ln -s "$f" "$stage/common/$(basename "$f")"; done
lsrc="$base/build/psp/thirdparty/luasocket/source/src"
cp "$lsrc/socket.lua" "$lsrc/ltn12.lua" "$lsrc/mime.lua" "$stage/common/"
for f in url http ftp tp smtp headers; do cp "$lsrc/$f.lua" "$stage/common/socket/"; done
cp "$(dirname "$0")/luastubs/socket/score.lua" "$stage/common/socket/score.lua"
mkdir -p "$stage/common/ssl"; cp "$(dirname "$0")/luastubs/ssl.lua" "$stage/common/ssl.lua"; cp "$(dirname "$0")/luastubs/ssl/https.lua" "$stage/common/ssl/https.lua"
cp "$(dirname "$0")/luastubs/mime/mcore.lua" "$stage/common/mime/mcore.lua"
mkdir -p "$stage/data" "$stage/libs" "$stage/settings" "$stage/cache" "$stage/clipboard"
cp -f "$base/build/psp/data/ca-bundle.crt" "$stage/data/" 2>/dev/null || true
mkdir -p "$stage/samples"
for f in juliet.epub leaves.epub sample.pdf paper.pdf 2col.pdf djvu3spec.djvu sample.txt; do cp -f "$ko/test/$f" "$stage/samples/" 2>/dev/null || true; done
echo "v2026.11-psp-dev" > "$stage/git-rev"
cp -f "$eboot" "$stage/EBOOT.PBP"
echo "staged in $stage"
