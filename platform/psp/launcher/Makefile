# KOReader launcher EBOOT for the PSP.
#   export PSPDEV=...; export PATH=$PSPDEV/bin:$PATH
#   make BASE=/path/to/koreader-base       (after `make TARGET=psp` + ninja in koreader-base)
all: EBOOT.PBP

BASE ?= $(error set BASE to the koreader-base checkout)
BUILD := $(BASE)/build/psp
STAGING := $(BUILD)/staging
NM = psp-nm

TARGET = koreader
KO_OBJS := $(filter-out %/unpack.c.obj %/input.c.obj %/djvu.c.obj %/wrap-mupdf.c.obj,$(shell find $(BUILD)/cmake/koreader -name '*.obj' 2>/dev/null))
STATIC_LIBS = \
  $(STAGING)/lib/libcrengine.a $(BUILD)/thirdparty/crengine/build/libcrengine.a \
  \
  $(STAGING)/lib/libharfbuzz.a $(STAGING)/lib/libfreetype.a $(STAGING)/lib/libfribidi.a $(STAGING)/lib/libunibreak.a \
  $(STAGING)/lib/libutf8proc.a $(STAGING)/lib/liblunasvg.a $(STAGING)/lib/libmd4c-html.a $(STAGING)/lib/libmd4c.a $(STAGING)/lib/libaes.a \
  $(STAGING)/lib/libarchive.a $(STAGING)/lib/liblzma.a $(STAGING)/lib/libzstd.a $(STAGING)/lib/libxxhash.a \
  $(STAGING)/lib/libwebpdemux.a $(STAGING)/lib/libwebp.a $(STAGING)/lib/libsharpyuv.a $(STAGING)/lib/libgif.a \
  $(STAGING)/lib/liblodepng.a $(STAGING)/lib/libturbojpeg.a $(STAGING)/lib/libjpeg.a $(STAGING)/lib/liblibpng16_static.a \
  $(STAGING)/lib/libbrotlidec.a $(STAGING)/lib/libbrotlicommon.a $(STAGING)/lib/libz.a \
  $(STAGING)/lib/libkoreader-lfs.a $(STAGING)/lib/lpeg.a $(STAGING)/lib/rapidjson.a $(STAGING)/lib/libsqlite3.a \
  $(STAGING)/lib/libluajit-5.1.a
STATIC_LIBS := $(wildcard $(STATIC_LIBS))
LUA_DIRS = --lua-dir $(BASE)/ffi --lua-dir $(abspath ../../../frontend) --lua-dir $(abspath ../../../plugins) --lua-dir $(abspath ../../..)/

# ENOSYS stubs for POSIX functions declared in the PSP posix cdecl but missing from libc (and not stubbed by hand).
posix_stubs_gen.c: psp_posix_stubs.o $(BASE)/ffi-cdecl/posix_h_other_mipsel.lua $(BASE)/utils/gen-psp-posix-stubs.py
	python3 $(BASE)/utils/gen-psp-posix-stubs.py $@ --nm $(NM) --cdecl $(BASE)/ffi-cdecl/posix_h_other_mipsel.lua \
	  --have $(shell psp-config --pspsdk-path)/lib/crt0_prx.o $(PSPDEV)/psp/lib/libc.a $(PSPDEV)/psp/lib/libm.a $(PSPDEV)/psp/lib/libpthread.a $(PSPDEV)/psp/lib/libSDL3.a psp_posix_stubs.o \
	  --success fsync

symtab.c: posix_stubs_gen.o $(KO_OBJS) $(STATIC_LIBS) $(BASE)/utils/gen-psp-symtab.py
	python3 $(BASE)/utils/gen-psp-symtab.py $@ --nm $(NM) --extra memcpy --extra memset $(LUA_DIRS) $(KO_OBJS) $(STATIC_LIBS) posix_stubs_gen.o psp_posix_stubs.o \
	  $(PSPDEV)/psp/lib/libSDL3.a $(PSPDEV)/psp/lib/libc.a $(PSPDEV)/psp/lib/libm.a

OBJS = main.o symtab.o psp_posix_stubs.o posix_stubs_gen.o $(KO_OBJS) $(if $(SCREENSHOTS),screenshot.o)
PSPSDK ?= $(shell psp-config --pspsdk-path)
INCDIR = $(STAGING)/include/luajit-2.1
CFLAGS = -O2 -G0 -Wall $(shell psp-pkg-config --cflags sdl3) -Wno-unused-function \
  $(if $(FORCE_LOG),-DKO_PSP_FORCE_LOG) $(if $(PRESS),-DKO_PSP_PRESS='"$(PRESS)"') $(if $(ARG1),-DKO_PSP_ARG1='"$(ARG1)"') \
  $(if $(SCRIPT),-DKO_PSP_SCRIPT='"$(SCRIPT)"') \
  $(if $(OPEN),-DKO_PSP_OPEN='"$(OPEN)"') \
  $(if $(SCREENSHOTS),-DKO_PSP_SCREENSHOTS -DKO_PSP_SCREENSHOTS_FIRST_MS=$(or $(SHOT_FIRST),20000) -DKO_PSP_SCREENSHOTS_INTERVAL_MS=$(or $(SHOT_INTERVAL),10000) -DKO_PSP_SCREENSHOTS_COUNT=$(or $(SHOT_COUNT),6))
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBDIR = $(STAGING)/lib
LIBS = -Wl,--start-group $(STATIC_LIBS) $(PSPDEV)/psp/lib/libsqlite3.a -Wl,--end-group \
       $(shell psp-pkg-config --libs sdl3) -lstdc++ -lsupc++ -lm -lpthread -lpspnet_inet
BUILD_PRX = 1
PSP_FW_VERSION = 661
PSP_LARGE_MEMORY = 1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = KOReader
PSP_EBOOT_ICON = ICON0.PNG

include $(PSPSDK)/lib/build.mak
