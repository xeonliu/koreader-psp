# KOReader 2014.11 launcher EBOOT for the PSP.
#   export PSPDEV=...; export PATH=$PSPDEV/bin:$PATH
#   make OLDBASE=/path/to/psp2014/base LIBS=/path/to/modern/koreader-base/build/psp
# (the static libraries - FreeType, libpng, jpeg, lodepng, lfs, ... - come from the modern koreader-base PSP build for now)
all: EBOOT.PBP

OLDBASE ?= $(error set OLDBASE to the 2014 koreader-base checkout)
LIBS_DIR ?= $(error set LIBS_DIR to a koreader-base PSP build directory (build/psp))
STAGING := $(LIBS_DIR)/staging
KOPSP := $(LIBS_DIR)/psp-posix/libkopsp.a
KO := $(abspath ../../..)
NM = psp-nm

TARGET = koreader
STATIC_LIBS = $(addprefix $(STAGING)/lib/, \
  libkoreader-lfs.a liblodepng.a libgif.a libturbojpeg.a libjpeg.a liblibpng16_static.a \
  libfreetype.a libbrotlidec.a libbrotlicommon.a libz.a libluajit-5.1.a lpeg.a)
LUA_DIRS = --lua-dir $(OLDBASE)/ffi --lua-dir $(KO)/frontend --lua-dir $(KO)/plugins --lua-dir $(KO)

posix_stubs_gen.c: $(KOPSP) $(OLDBASE)/ffi/posix_h.lua $(OLDBASE)/utils/gen-psp-posix-stubs.py
	python3 $(OLDBASE)/utils/gen-psp-posix-stubs.py $@ --nm $(NM) --cdecl $(OLDBASE)/ffi/posix_h.lua \
	  --have $(PSPDEV)/psp/lib/libc.a $(PSPDEV)/psp/lib/libm.a $(PSPDEV)/psp/lib/libpthread.a $(PSPDEV)/psp/lib/libSDL.a $(KOPSP) --success fsync

symtab.c: posix_stubs_gen.o $(STATIC_LIBS) $(KOPSP) $(OLDBASE)/utils/gen-psp-symtab.py
	python3 $(OLDBASE)/utils/gen-psp-symtab.py $@ --nm $(NM) --extra memcpy --extra memset $(LUA_DIRS) $(STATIC_LIBS) \
	  posix_stubs_gen.o $(KOPSP) $(PSPDEV)/psp/lib/libSDL.a $(PSPDEV)/psp/lib/libc.a $(PSPDEV)/psp/lib/libm.a

OBJS = main.o symtab.o posix_stubs_gen.o $(if $(SCREENSHOTS),screenshot.o)
PSPSDK ?= $(shell psp-config --pspsdk-path)
INCDIR = $(STAGING)/include/luajit-2.1
CFLAGS = -O2 -G0 -Wall $(shell psp-pkg-config --cflags sdl) -Wno-unused-function \
  $(if $(ARG1),-DKO_PSP_ARG1='"$(ARG1)"') $(if $(SCRIPT),-DKO_PSP_SCRIPT='"$(SCRIPT)"') $(if $(OPEN),-DKO_PSP_OPEN='"$(OPEN)"') \
  $(if $(SCREENSHOTS),-DKO_PSP_SCREENSHOTS -DKO_PSP_SCREENSHOTS_FIRST_MS=$(or $(SHOT_FIRST),20000) -DKO_PSP_SCREENSHOTS_INTERVAL_MS=$(or $(SHOT_INTERVAL),10000) -DKO_PSP_SCREENSHOTS_COUNT=$(or $(SHOT_COUNT),6))
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBDIR = $(STAGING)/lib
LDFLAGS = -Wl,--wrap=write
LIBS = -Wl,--start-group $(STATIC_LIBS) $(KOPSP) -Wl,--end-group $(shell psp-pkg-config --libs sdl) -lstdc++ -lsupc++ -lm -lpthread
BUILD_PRX = 1
PSP_FW_VERSION = 661
ifneq ($(LARGE_MEMORY),)
PSP_LARGE_MEMORY = 1
endif
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = KOReader 2014.11
PSP_EBOOT_ICON = ICON0.PNG

include $(PSPSDK)/lib/build.mak
