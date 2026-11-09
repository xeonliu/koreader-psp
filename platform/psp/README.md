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

## `koreader-base` PSP target

`make TARGET=psp setup` in `koreader-base` configures the cross build (static/monolibtic, no SIMD, no PIC). zlib and libjpeg-turbo build.

## LuaJIT (Plan A): builds, untested

KOReader relies on LuaJIT's FFI everywhere, and LuaJIT does not support the PSP out of the box
(`#error "Only o32 ABI supported for MIPS32"`; psp-gcc targets MIPS EABI; the Allegrex FPU is single-precision only).
`koreader-base/thirdparty/luajit/psp-eabi-softfp.patch` accepts the ABI, forces LuaJIT's soft-float mode and no-ops the cache sync.
It is built as a static interpreter: JIT disabled, system allocator, FFI enabled (`LJ_HASFFI=1`).

`luajit-test/` is a console-only EBOOT that runs `test.lua` (arithmetic, strings, `bit`, a timing loop and FFI structs/calls) on it:

    (cd koreader-base && make TARGET=psp && cd build/psp/cmake && ninja luajit)
    cd luajit-test && make STAGING=/path/to/koreader-base/build/psp/staging

**It has been compiled and linked but NOT run.** Run it in PPSSPP or on hardware and check the output. Open questions:
does the VM use any instruction the Allegrex lacks (EABI vs o32 calling sequences inside `vm_mips.dasc`), and is soft-float speed acceptable?

Fallbacks if it does not run: PUC Lua with a LuaJIT-compatible `ffi` module, or porting the interpreter VM to the Allegrex.
