ifeq ($V, 1)
	VERBOSE =
else
	VERBOSE = @
endif

include config.mk

SRC := \
	src/main.c \
	src/patch.c \
	src/get_path.c \
	src/video.c \
	src/sync.c

SRC_ASM := \
	src/fov.S \
	src/video_init.S

ifeq ($(WRAP_DINPUT), 1)
	DEF += "-DWRAP_DINPUT"
	TARGET := build/dinput8.dll
else
	TARGET := build/d3d8.dll
endif

OBJ := $(SRC:src/%.c=build/%.o)
OBJ += $(SRC_ASM:src/%.S=build/%.o)
DEP := $(OBJ:%.o=%.d)
INC := -Iinclude

all: $(TARGET) | build

.PHONY: clean all
.SUFFIXES:

-include $(DEP)

build:
	@mkdir -p build

build/%.o: src/%.S
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CC) $(CFLAGS) $(CFLAGS_OPT) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

build/%.o: src/%.c
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CC) $(CFLAGS) $(CFLAGS_OPT) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

$(TARGET).sym: $(OBJ)
	@echo ld $(notdir $@)
	$(VERBOSE) $(ENV) $(LD) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

$(TARGET): $(TARGET).sym
	@echo strip $(notdir $@)
	$(VERBOSE) $(ENV) $(STRIP) $(TARGET).sym -o $@

clean:
	@rm -rf build
