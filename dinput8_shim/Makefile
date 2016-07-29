ifeq ($V, 1)
	VERBOSE =
else
	VERBOSE = @
endif

include ../config.mk

SRC := \
	main.c \
	get_path.c \
	patch.c

SRC_ASM := \
	export.S

TARGET := build/dinput8.dll

VPATH += ../src
OBJ := $(SRC:%.c=build/%.o)
OBJ += $(SRC_ASM:%.S=build/%.o)
DEP := $(OBJ:%.o=%.d)
INC := -I../include

all: $(TARGET) | build

.PHONY: clean all
.SUFFIXES:

-include $(DEP)

build:
	@mkdir -p build

build/%.o: %.S
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CC) $(CFLAGS) $(CFLAGS_OPT) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

build/%.o: %.c
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CC) $(CFLAGS) $(CFLAGS_OPT) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

$(TARGET).sym: $(OBJ)
	@echo ld $(notdir $@)
	$(VERBOSE) $(ENV) $(LD) $(LDFLAGS) -o $@ $(OBJ)

$(TARGET): $(TARGET).sym
	@echo strip $(notdir $@)
	$(VERBOSE) $(ENV) $(STRIP) $(TARGET).sym -o $@

clean:
	@rm -rf build
