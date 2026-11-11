# KOReader 2018.12 launcher EBOOT for the PSP.
#   export PSPDEV=...; export PATH=$PSPDEV/bin:$PATH
#   make BASE=/path/to/2018/koreader-base PREBUILT=/path/to/prebuilt/psp/staging
all: EBOOT.PBP

BASE ?= $(error set BASE to the 2018 koreader-base checkout (built with Makefile.psp))
PREBUILT ?= $(error set PREBUILT to the PSP staging directory with the prebuilt libraries)
OUT := $(BASE)/build/psp
KO := $(abspath ../../..)
NM = psp-nm
comma := ,
PSPSDK ?= $(shell psp-config --pspsdk-path)

TARGET = koreader
MAIN_OBJS := $(wildcard $(OUT)/main-obj/*.o)
STATIC_LIBS = $(addprefix $(PREBUILT)/lib/, \
  libkoreader-lfs.a lpeg.a liblodepng.a libgif.a libturbojpeg.a libjpeg.a liblibpng16_static.a \
  libharfbuzz.a libfreetype.a libbrotlidec.a libbrotlicommon.a libz.a) $(OUT)/lib/libluajit.a
KOPSP := $(OUT)/lib/libkopsp.a
LIBC_LIBS := $(addprefix $(PSPDEV)/psp/lib/,libc.a libcglue.a libpthread.a libpthreadglue.a)
SDL_LIB := $(PSPDEV)/psp/lib/libSDL2.a
LUA_DIRS = --lua-dir $(BASE)/ffi --lua-dir $(KO)/frontend --lua-dir $(KO)/plugins --lua-dir $(KO)

# What the PRX modules need from the main program.
module-imports.lst: $(wildcard $(OUT)/modules/*.imports)
	cat /dev/null $^ | sort -u > $@

posix_stubs_gen.c: $(KOPSP) $(BASE)/ffi/posix_h_psp.lua $(BASE)/utils/gen-psp-posix-stubs.py
	python3 $(BASE)/utils/gen-psp-posix-stubs.py $@ --nm $(NM) --cdecl $(BASE)/ffi/posix_h_psp.lua \
	  --have $(LIBC_LIBS) $(PSPDEV)/psp/lib/libm.a $(PSPDEV)/psp/lib/libpthread.a $(SDL_LIB) $(KOPSP) --success fsync

symtab.c: posix_stubs_gen.o module-imports.lst $(MAIN_OBJS) $(STATIC_LIBS) $(KOPSP) $(BASE)/utils/gen-psp-symtab.py
	python3 $(BASE)/utils/gen-psp-symtab.py $@ --nm $(NM) --extra memcpy --extra memset --extra _impure_ptr --extra sceKernelTotalFreeMemSize --extra sceKernelMaxFreeMemSize --extra mallinfo --extra-file module-imports.lst \
	  $(LUA_DIRS) $(MAIN_OBJS) $(STATIC_LIBS) posix_stubs_gen.o $(KOPSP) $(SDL_LIB) $(LIBC_LIBS) $(PSPDEV)/psp/lib/libm.a $(PSPSDK)/lib/libpspuser.a $(EXTRA_SYM_LIBS)

OBJS = main.o ko_modules.o memdiag.o symtab.o posix_stubs_gen.o $(MAIN_OBJS) $(if $(SCREENSHOTS),screenshot.o)
INCDIR = $(OUT)/include/luajit $(BASE)/toolchain/psp/prx
CFLAGS = -O2 -G0 -Wall $(shell psp-pkg-config --cflags sdl2) -Wno-unused-function \
  $(if $(MEMTRACE),-DKO_PSP_MEMTRACE=$(MEMTRACE)) $(if $(MODULE_RESERVE_KB),-DKO_PSP_MODULE_RESERVE_KB=$(MODULE_RESERVE_KB)) $(if $(ARG1),-DKO_PSP_ARG1='"$(ARG1)"') $(if $(SCRIPT),-DKO_PSP_SCRIPT='"$(SCRIPT)"') $(if $(OPEN),-DKO_PSP_OPEN='"$(OPEN)"') \
  $(if $(SCREENSHOTS),-DKO_PSP_SCREENSHOTS -DKO_PSP_SCREENSHOTS_FIRST_MS=$(or $(SHOT_FIRST),20000) -DKO_PSP_SCREENSHOTS_INTERVAL_MS=$(or $(SHOT_INTERVAL),10000) -DKO_PSP_SCREENSHOTS_COUNT=$(or $(SHOT_COUNT),6))
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBDIR = $(OUT)/lib
LIBS = -Wl,--wrap=_malloc_r,--wrap=_realloc_r,--wrap=_calloc_r $(if $(MEMTRACE),-Wl$(comma)--wrap=malloc$(comma)--wrap=calloc$(comma)--wrap=realloc) -Wl,--start-group $(STATIC_LIBS) $(KOPSP) -Wl,--end-group $(shell psp-pkg-config --libs --static sdl2) -lstdc++ -lsupc++ -lm -lpthread
BUILD_PRX = 1
PSP_FW_VERSION = 661
ifneq ($(LARGE_MEMORY),)
PSP_LARGE_MEMORY = 1
endif
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = KOReader 2018.12
PSP_EBOOT_ICON = ICON0.PNG

include $(PSPSDK)/lib/build.mak

ko_modules.o: ko_libmap.inc ko_modules.h
main.o memdiag.o: ko_modules.h FORCE
FORCE:
symtab.c posix_stubs_gen.c: Makefile
