# Console-only LuaJIT test EBOOT (LuaJIT interpreter, JIT disabled). export PSPDEV=... PATH=$PSPDEV/bin:$PATH
all: EBOOT.PBP

TARGET = luajit_psp_test
OBJS = main.o test_lua.o

# Point at koreader-base's PSP staging dir: make STAGING=/path/to/koreader-base/build/psp/staging
STAGING ?= $(error set STAGING to koreader-base/build/psp/staging (make TARGET=psp luajit))
PSPSDK ?= $(shell psp-config --pspsdk-path)
INCDIR = $(STAGING)/include/luajit-2.1
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBDIR = $(STAGING)/lib
LIBS = -lluajit-5.1 -lm -lpspdebug -lpspdisplay -lpspge -lpspctrl

BUILD_PRX = 1
PSP_FW_VERSION = 661
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = LuaJIT PSP test

test_lua.c: test.lua
	( echo 'const char test_lua[] ='; sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' $< ; echo ';' ) > $@

include $(PSPSDK)/lib/build.mak
