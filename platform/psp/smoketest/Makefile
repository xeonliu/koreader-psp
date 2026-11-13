# Build the PSP smoke-test EBOOT. Needs the pspdev toolchain: export PSPDEV=/path/to/pspdev PATH=$PSPDEV/bin:$PATH
all: EBOOT.PBP

TARGET = koreader_psp_smoketest
OBJS = main.o smoketest_lua.o

PSPSDK ?= $(shell psp-config --pspsdk-path)
INCDIR =
CFLAGS = -O2 -G0 -Wall $(shell psp-pkg-config --cflags sdl3)
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBDIR =
LDFLAGS =
LIBS = $(shell psp-pkg-config --libs sdl3) -llua -lm

BUILD_PRX = 1
PSP_FW_VERSION = 661
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = KOReader PSP smoke test

# Embed the Lua script as a C string.
smoketest_lua.c: smoketest.lua
	( echo 'const char smoketest_lua[] ='; sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' $< ; echo ';' ) > $@

include $(PSPSDK)/lib/build.mak
