# KOReader 2018.12 on the Sony PSP

A port of KOReader v2018.12 to the PSP with the pspdev toolchain and SDL2. The launcher (`EBOOT.PBP`) is small and
only holds what the user interface needs; each document engine is a PRX module in `modules/` that is loaded the
first time a document needs it.

Status: runs in PPSSPP (24 MB PSP-1000 profile): file manager, EPUB/TXT/HTML through `cre.prx`, PDF through
`mupdf.prx`. **Not tested on real hardware yet.**

## What is where

| Piece | Size | Loaded |
|---|---|---|
| `EBOOT.PBP`: LuaJIT (interpreter + FFI), SDL2, FreeType, HarfBuzz, libpng, libjpeg-turbo, zlib, giflib, lodepng, lfs, blitbuffer | 5.3 MB file, 5.9 MB in memory | at boot |
| `modules/cre.prx`: CREngine (EPUB, FB2, HTML, TXT, RTF, ...) | 1.9 MB | first such document |
| `modules/mupdf.prx`: MuPDF 1.13 (PDF, XPS, CBZ, HTML dictionary widget) | 3.6 MB | first such document |

The modules do not contain FreeType, libjpeg, zlib, libc, ...: they call the copies in the launcher. Their `malloc`
is the launcher's heap.

Memory: the launcher keeps part of the free memory outside its C heap for modules (2.5 MB on 24 MB models, 10 MB on
52 MB models; see `launcher/heap.c`). On a 24 MB PSP-1000, EPUB, FB2, HTML and TXT work (`cre.prx`), PDF does not
(`mupdf.prx` does not fit beside the Lua heap; tested only at 52 MB, where PDF renders). If a module does not fit,
opening the document fails with "not enough memory left for modules".

## How modules work

See `toolchain/psp/prx/ko_prx.h` in koreader-base. In short:

- `mkprx.py` links a library with `-nostdlib` into a relocatable PRX. Every function it needs from outside goes
  through a generated trampoline; `module_start` fills the trampolines by name from the launcher's symbol table and
  returns a table of the module's exported symbols.
- LuaJIT's `ffi.load()` and `require()` ask the launcher (`launcher/ko_modules.c`), which maps library names to
  modules (`launcher/ko_libmap.inc`) and loads `modules/NAME.prx` with `sceKernelLoadModule`.
- `NAME.imports` (written next to each PRX) lists what the launcher must export; the launcher's Makefile feeds these
  lists to its symbol table generator. Rebuild the launcher after changing a module.

## Building

    export PSPDEV=/path/to/pspdev PATH=$PSPDEV/bin:$PATH
    # koreader-base, branch claude/psp-2018-12; PREBUILT is the staging directory of the modern koreader-base PSP
    # build (branch claude/psp-porting-sdl-mm2bsa), which provides FreeType, HarfBuzz, libpng, libjpeg-turbo, ...
    make -C $BASE -f Makefile.psp PREBUILT=$PREBUILT all cre mupdf
    make -C platform/psp/launcher BASE=$BASE PREBUILT=$PREBUILT
    KO_PSP_FONTS=/path/to/fonts platform/psp/stage.sh OUT_DIR $BASE /path/to/prebuilt/common

Copy `OUT_DIR` to `ms0:/PSP/GAME/KOReader/`.

Launcher build options: `SCREENSHOTS=1` (dump the frame buffer to `host0:/` in PPSSPP), `OPEN=file`, `ARG1=dir`,
`SCRIPT=file.lua`, `MODULE_RESERVE_KB=n`, `MEMTRACE=kb` (log allocations of at least that size with their caller).
`SCRIPT=test_prx.lua` with koreader-base's `kotest` module is a self-test of the module mechanism.

## Differences from other platforms

- No k2pdfopt/Leptonica: PDFs open without reflow, auto-crop and OCR.
- CREngine without CHM and MS Word (antiword) support.
- No network (LuaSocket/LuaSec are stand-ins that fail cleanly), no SQLite (statistics and cover browser plugins
  do not load), no DjVu yet.
- UI icons are decoded with lodepng/turbojpeg so that MuPDF is not needed at boot.
- The screen is used in landscape (480x272) by default.
