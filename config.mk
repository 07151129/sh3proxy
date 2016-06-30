GCC_PATH ?= /usr/local/Cellar/i686-w64-mingw32-gcc/4.9.3
BINUTILS_PATH ?= /usr/local/Cellar/i686-w64-mingw32-binutils/2.25.1
MINGW32_PATH ?= $(GCC_PATH)/i686-w64-mingw32

CC := $(GCC_PATH)/bin/i686-w64-mingw32-gcc
LD := $(CC)
STRIP := $(BINUTILS_PATH)/bin/i686-w64-mingw32-strip

ARCH := 32

CFLAGS_OPT := \
    -Os

CFLAGS := \
    -m$(ARCH) \
    -std=c99 \
    -shared \
    -fdiagnostics-color=auto

LDFLAGS := \
    -m$(ARCH) \
    -shared \
    -Wl,--enable-auto-import \
    --enable-stdcall-fixup
