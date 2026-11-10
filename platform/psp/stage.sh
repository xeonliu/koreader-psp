#!/usr/bin/env bash
# Assemble a runnable KOReader v2014.11 directory for the PSP (or for PPSSPP's --root).
#   platform/psp/stage.sh STAGE_DIR /path/to/2014/koreader-base [EBOOT.PBP] [FONTS_DIR]
set -euo pipefail
stage="$1"; base="$2"; eboot="${3:-$(dirname "$0")/launcher/EBOOT.PBP}"; fonts="${4:-}"
ko="$(cd "$(dirname "$0")/../.." && pwd)"
mkdir -p "$stage"
link() { rm -rf "$stage/$2"; ln -s "$1" "$stage/$2"; }
for f in reader.lua defaults.lua; do link "$ko/$f" "$f"; done
for d in frontend plugins resources l10n; do link "$ko/$d" "$d"; done
link "$base/ffi" ffi
# Network modules are stand-ins (no network yet).
rm -rf "$stage/common"; mkdir -p "$stage/common"
cp -r "$(dirname "$0")/luastubs/." "$stage/common/"
[ -n "$fonts" ] && link "$fonts" fonts || mkdir -p "$stage/fonts"
mkdir -p "$stage/data" "$stage/libs" "$stage/cache" "$stage/history" "$stage/clipboard" "$stage/samples"
for f in juliet.epub leaves.epub sample.pdf paper.pdf 2col.pdf djvu3spec.djvu sample.txt; do
    for d in "$ko/test" /home/user/koreader-psp/test; do [ -f "$d/$f" ] && cp -f "$d/$f" "$stage/samples/" && break; done
done
echo "${KO_PSP_VERSION:-v2014.11-psp}" > "$stage/git-rev"
cp -f "$eboot" "$stage/EBOOT.PBP"
echo "staged in $stage"
