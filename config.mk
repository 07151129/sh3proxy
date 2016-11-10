GCC_PATH ?=
BINUTILS_PATH ?= 
MINGW32_PATH ?= 

CC := i686-w64-mingw32-gcc
LD := $(CC)
STRIP := i686-w64-mingw32-strip

ARCH := 32

CFLAGS_OPT := \
    -Os

CFLAGS := \
    -m$(ARCH) \
    -std=c99 \
    -shared \
    -fdiagnostics-color=auto \
    -Wall

LDFLAGS := \
    -m$(ARCH) \
    -shared \
    -Wl,--enable-auto-import \
    -Wl,--enable-stdcall-fixup
