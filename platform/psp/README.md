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

`launcher/` builds the real thing: one static `EBOOT.PBP` (~16 MB) holding LuaJIT and the C dependencies of the core reader
(CREngine, FreeType, HarfBuzz, libarchive, image decoders, sqlite, ...). KOReader's own Lua (`reader.lua` and friends) is staged next to it.
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

### Status (verified in PPSSPP only; not yet confirmed on real hardware)
The build is the core reader: one static `EBOOT.PBP` (~16 MB) with CREngine. It needs a PSP-2000 or later (it asks for the large
memory layout); a PSP-1000 does not have enough RAM.

Working in PPSSPP: boot, quickstart guide, file manager, EPUB, MOBI, plain text, menus, page turns with the buttons.
Not built: PDF (MuPDF), DjVu, k2pdfopt/OCR, network.

Buttons (KOReader's own SDL gamepad map, `frontend/device/sdl/gamepad.lua`): D-pad/analog stick = move focus, Cross = select,
Circle = back, Triangle = context menu, Start = menu, L/R = previous/next page.

For real hardware the launcher
* masks FPU exceptions (the Allegrex traps on them, PPSSPP does not),
* uses a 1 MB main stack and keeps 2 MB outside the C heap for thread stacks,
* pools file handles (`fdpool.c`): the firmware allows only about 16 open files and KOReader keeps many more open (fonts above
  all), so read-only files get virtual handles and only the 6 most recently used are really open,
* writes stdout/stderr to `koreader.log` next to the EBOOT when run from `ms0:`/`ef0:` (each line is flushed to the card, so the
  log survives a crash), and shows a fatal Lua error on screen for 15 seconds.

To make a folder for the memory stick, copy the stage directory with links resolved (`cp -rL stage KOReader`), drop
`resources/fonts` (the same files as `fonts/`), and put it in `ms0:/PSP/GAME/`.

Known problems:
* sqlite databases cannot be created (statistics, cover browser): `ljsqlite3[cantopen]`. The plugins fail to load; reading is unaffected.
* Speed has not been measured on hardware (the VM does doubles in software); opening a book takes 5-15 s in PPSSPP.

Extra development switches: `PRESS=ms:button,ms:button` (scripted gamepad presses, with `SCREENSHOTS=1`), `FORCE_LOG=1` (write
`koreader.log` in the emulator too), `MAX_OPEN=n` (refuse more than n open files, like the firmware does).

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
