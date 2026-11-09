# KOReader on the Sony PSP (work in progress)

Status: **build-system groundwork + a smoke-test EBOOT. This is not KOReader yet, and nothing here has been run on hardware or an emulator.**

## Toolchain

Install [pspdev](https://github.com/pspdev/pspdev) (release tarball), then:

    export PSPDEV=/path/to/pspdev
    export PATH=$PSPDEV/bin:$PATH

pspdev ships SDL3 (`libSDL3.a`), freetype, libpng, libjpeg, zlib, sqlite3 and Lua 5.4 for PSP.

## Smoke test (`smoketest/`)

SDL3 (480x272) + SDL3 gamepad + embedded Lua 5.4, packed as `EBOOT.PBP`:

    cd smoketest && make

Copy `EBOOT.PBP` to `ms0:/PSP/GAME/KOReaderSmoke/` (or open it in PPSSPP). It shows the pressed buttons, FPS, the Lua heap size,
and a box you can move with the D-pad or analog stick. Start+Select quits.

For automated screenshots, build with `CFLAGS="... -DSMOKE_AUTOEXIT_MS=3000"` (see `smoketest/main.c`). The app then dumps the PSP frame buffer to
`host0:/smoketest_screen.bmp` and exits, so `ppsspp-headless EBOOT.PBP --root $PWD --graphics=software` produces the image.

## `koreader-base` PSP target

`make TARGET=psp setup` in `koreader-base` configures the cross build (static/monolibtic, no SIMD, no PIC). zlib and libjpeg-turbo build.

## KOReader on the PSP (`launcher/`)

`launcher/` builds the real thing: one `EBOOT.PBP` (~27 MB) holding a static LuaJIT, KOReader's own Lua (`reader.lua` and friends),
and every C dependency (FreeType, HarfBuzz, MuPDF, CREngine, DjVuLibre, k2pdfopt/Leptonica/Tesseract, libarchive, sqlite, ...).
SDL3 (pspdev's) provides the 480x272 screen and the gamepad; KOReader's existing SDL device code is reused unchanged.

    # 1. koreader-base: toolchain + all thirdparty libraries (needs pspdev in PATH, PSPDEV set)
    cd koreader-base && make TARGET=psp setup && (cd build/psp/cmake && ninja -k0 all)
    #    (if GitHub tarball downloads are blocked: utils/fetch-git-archives.sh + KO_LOCAL_ARCHIVES, see its header)
    # 2. launcher EBOOT
    cd koreader-psp/platform/psp/launcher && make BASE=/path/to/koreader-base
    # 3. KOReader's Lua tree + fonts + sample documents next to the EBOOT
    ../stage.sh /path/to/stage /path/to/koreader-base

Copy the stage directory to `ms0:/PSP/GAME/KOReader/` (or run it with `ppsspp-headless EBOOT.PBP --root STAGE`).

Development switches for `make` (see `launcher/Makefile`): `OPEN=samples/x.epub` (open a book at start), `ARG1=-d` (debug logging),
`SCRIPT=test.lua` (run a script instead of reader.lua), `SCREENSHOTS=1 SHOT_FIRST=ms SHOT_INTERVAL=ms SHOT_COUNT=n` (dump frames to
`host0:/shot_N.bmp`, which is how the emulator screenshots were made).

How it fits together:
* `ffi.C` / `ffi.load` resolve against a table generated at link time (`koreader-base/utils/gen-psp-symtab.py`); there is no dynamic loader.
* `package.loadlib` is replaced by a lookup of the statically linked `luaopen_*` functions.
* POSIX functions newlib lacks are provided by `launcher/psp_posix_stubs.c` (real where it matters) and generated ENOSYS stubs.
* `koreader-base/ffi-cdecl/posix_h_other_mipsel.lua` holds the PSP's C declarations (types/constants), merged into `ffi/posix_h.lua`.
* No network: `socket`, `ssl` and `mime` are small Lua stand-ins (`luastubs/`), so network features fail cleanly.

### Status (all verified in PPSSPP only; nothing has run on real hardware)
Working: boot, file manager, EPUB (CREngine), plain text (CREngine), PDF (MuPDF).
Known problems:
* `leaves.epub` and DjVu crash with wild memory accesses (DjVuLibre uses C++ exceptions; unwinding probably does not work in the PRX).
* sqlite databases cannot be created (statistics, cover browser): `ljsqlite3[cantopen]`.
* Memory: the EBOOT is 27 MB and KOReader uses a lot of RAM; PPSSPP is run in large-memory mode. A PSP-1000 will not have enough.
* Speed has not been measured on hardware (the VM does doubles in software).
* Not built: libressl, libzmq/czmq, luasocket (stand-in used), sdcv (needs GLib).
* Input: the gamepad is read through KOReader's SDL device layer; no PSP-specific key map yet.

## LuaJIT (Plan A): runs in PPSSPP

LuaJIT does not support the PSP out of the box (MIPS EABI, single-precision FPU). `koreader-base/thirdparty/luajit/psp-eabi-softfp.patch`
makes it work as a static, soft-float interpreter (JIT off, FFI on):

* accepts the EABI and forces LuaJIT's soft-float mode; no-op instruction cache sync;
* `buildvm` no longer emits `.abicalls` (a lone PIC object made the linker emit unrelocated stubs in the PRX);
* the interpreter frame owns the 16-byte area (EABI has no caller-owned argument home area);
* the 5th argument of `lj_meta_arith`/`lj_meta_bitop` is passed in `a4`;
* soft-float int->double conversion uses a software `clz` (the Allegrex encodes `clz` differently from MIPS32);
* FFI calls follow the EABI: `a0`-`a7`, stack arguments at `sp+0`, `float` arguments in `f12`-`f19` and results in `f0`;
* `ffi.C` / `ffi.load` use a static symbol table (see above).

`luajit-test/` runs `test.lua` (arithmetic, 32-bit overflow, tables, GC, coroutines, FFI structs/calls with many and float arguments)
and prints PASS/FAIL:

    (cd koreader-base && make TARGET=psp && cd build/psp/cmake && ninja luajit)
    cd luajit-test && make STAGING=/path/to/koreader-base/build/psp/staging
    ppsspp-headless $PWD/EBOOT.PBP --graphics=software -l 2>&1 | grep stdout     # nix: nixpkgs#ppsspp-sdl
