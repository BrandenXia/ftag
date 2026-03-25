MODE ?= DEBUG
CC ?= clang

CFLAGS := -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wimplicit-fallthrough -std=c23
CFLAGS_DEBUG := -g -O0
CFLAGS_RELEASE := -O3

ifeq ($(MODE), DEBUG)
	CFLAGS += $(CFLAGS_DEBUG)
else ifeq ($(MODE), RELEASE)
	CFLAGS += $(CFLAGS_RELEASE)
else
	$(error Invalid MODE: $(MODE). Use DEBUG or RELEASE.)
endif

INCLUDE_DIRS := include external/BLAKE3/c
INCLUDE_FLAGS := $(patsubst %,-I%, $(INCLUDE_DIRS))
LIBS_FILES := external/BLAKE3/c/build/libblake3.a
LIB_FLAGS := -lsqlite3 -Lexternal/BLAKE3/c/build -lblake3
BUILD_DIR ?= build
BUILD_DIR_SLASH := $(BUILD_DIR)/
TARGET := $(BUILD_DIR)/ftag
OBJS := $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(wildcard src/*.c))
DEPS := $(patsubst src/%.c, $(BUILD_DIR)/%.d, $(wildcard src/*.c))

.PHONY: all clean-build clean-exe clean-compile-commands clean
.PRECIOUS: $(BUILD_DIR_SLASH) $(BUILD_DIR)%/

all: $(TARGET)

$(BUILD_DIR_SLASH):
	mkdir -p $@

-include $(DEPS)

external/BLAKE3/c/build/libblake3.a:
	cmake -S external/BLAKE3/c -B external/BLAKE3/c/build
	cmake --build external/BLAKE3/c/build

$(BUILD_DIR)/%.o: src/%.c Makefile | $(BUILD_DIR_SLASH)
	$(CC) $(CFLAGS) -MMD -MP $(INCLUDE_FLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(LIBS_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LIB_FLAGS)

compile_commands.json: clean
	bear -- make

clean-build:
	rm -rf $(BUILD_DIR)

clean-exe: 
	rm -f $(TARGET)

clean-compile-commands:
	rm -f compile_commands.json

clean: clean-build clean-exe clean-compile-commands
