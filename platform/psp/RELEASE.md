# KOReader for PSP — v0.1.0 alpha (preview)

**An experimental port.** KOReader's real Lua frontend running on the Sony PSP through a static LuaJIT and SDL3.
It has been run in the PPSSPP emulator only. **It has not been tested on real hardware.**

## What works (in PPSSPP)
* Boot, file manager, settings menus.
* EPUB and plain text (CREngine), PDF (MuPDF).
* The PSP gamepad through KOReader's SDL gamepad layer.

## What does not work / known problems
* **Memory:** the EBOOT is ~29 MB and KOReader needs a lot of RAM. A PSP-1000 (32 MB) will almost certainly not run it.
  A PSP-2000/3000/Go/E1000 (64 MB) *might*. The EBOOT requests large-memory mode.
* **Speed:** LuaJIT runs as a soft-float interpreter (the PSP CPU has no double-precision FPU): expect slow start-up (the
  first boot builds a font list) and slow page turns. Real-hardware speed is unknown.
* DjVu files and `leaves.epub` (from the sample set) crash.
* No network (Wi-Fi, OPDS, sync, updates): the socket/TLS modules are stubs.
* sqlite-backed plugins (statistics, cover browser, vocabulary builder) cannot create their databases.
* No dictionary support, no PSP-specific key map or e-ink/frontlight handling.
* Needs custom firmware (e.g. ARK-4, PRO) to run homebrew.

## Install
1. Copy the `KOReader` folder to `ms0:/PSP/GAME/` on the Memory Stick (so you have `ms0:/PSP/GAME/KOReader/EBOOT.PBP`).
2. Start it from *Game → Memory Stick*. (Emulator: open `KOReader/EBOOT.PBP` in PPSSPP.)
3. Put your books anywhere on the Memory Stick; browse to them from KOReader's file manager. A few samples are in `KOReader/samples/`.

## Controls (KOReader's SDL gamepad map)
| PSP | Action |
|---|---|
| D-pad | move focus / (in a book) navigate |
| ✕ | select (press) |
| ○ | back |
| △ | context menu |
| L / R | previous / next page |
| Select | previous page |
| Start | menu |

## Build
See `platform/psp/README.md`. Release build: `make BASE=…` in `platform/psp/launcher` (without the `SCREENSHOTS`/`OPEN`/`ARG1`/`SCRIPT`
development switches), then `KO_PSP_VERSION=v2026.11-1-psp-alpha platform/psp/stage.sh DIR …`.
