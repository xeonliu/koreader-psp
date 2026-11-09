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

## LuaJIT (Plan A): runs in PPSSPP (emulator only; not tested on hardware)

KOReader relies on LuaJIT's FFI everywhere, and LuaJIT does not support the PSP out of the box (MIPS EABI, single-precision FPU).
`koreader-base/thirdparty/luajit/psp-eabi-softfp.patch` makes it work as a static, soft-float interpreter (JIT off, FFI on):

* accepts the EABI and forces LuaJIT's soft-float mode; no-op instruction cache sync;
* `buildvm` no longer emits `.abicalls` for the VM object (a lone PIC object made the linker emit unrelocated stubs in the PRX);
* the interpreter frame owns the 16-byte area (EABI has no caller-owned argument home area; the VM stored `SAVE_L` etc. into the caller's frame);
* the 5th argument of `lj_meta_arith`/`lj_meta_bitop` is passed in `a4` (EABI) instead of on the stack (o32).

`luajit-test/` runs `test.lua` (arithmetic, 32-bit overflow, tables, GC, coroutines, FFI structs) and prints PASS/FAIL. Run it headless:

    (cd koreader-base && make TARGET=psp && cd build/psp/cmake && ninja luajit)
    cd luajit-test && make STAGING=/path/to/koreader-base/build/psp/staging
    ppsspp-headless $PWD/EBOOT.PBP --graphics=software -l 2>&1 | grep stdout     # nix: nixpkgs#ppsspp-sdl

Result: all checks pass; a 2M-iteration loop takes ~1 s of emulated time.

### Known gaps
* `ffi.C.<name>` / `ffi.load()`: no symbol lookup on this OS (needs a generated static symbol table). Required before KOReader's `ffi/*.lua` can run.
* FFI *calls into C* (`lj_ccall.c`) and callbacks still use the o32 convention (4 register args, 16-byte home area, floats in GPRs).
  EABI passes 8 register args; this must be adapted before calling C functions with more than 4 arguments.
* Performance on real hardware is unmeasured.
