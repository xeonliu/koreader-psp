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

## Blocker: LuaJIT

KOReader relies on LuaJIT's FFI everywhere, but LuaJIT does not support the PSP CPU/ABI:

* `lj_arch.h`: `#error "Only o32 ABI supported for MIPS32"` (psp-gcc targets MIPS EABI).
* Its assembly VM (`vm_mips.dasc`) assumes a double-precision FPU; the Allegrex FPU is single-precision only.

Options, roughly in order of preference:

1. Run KOReader's Lua on PUC Lua 5.1/5.4 and provide a LuaJIT-compatible `ffi` module (e.g. cffi-lua + a libffi port for MIPS EABI, or a purpose-built static binding layer).
2. Port the LuaJIT interpreter VM to the Allegrex (large; the JIT would stay disabled).
3. Reduce scope: a native (C) reader front end with Lua only for UI logic.
