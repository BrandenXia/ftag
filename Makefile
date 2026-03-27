MODE ?= DEBUG
CC ?= clang

CFLAGS := -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wimplicit-fallthrough -std=c23
CFLAGS_DEBUG := -g -O0
CFLAGS_RELEASE := -O3

ifeq ($(MODE), DEBUG)
	CFLAGS += $(CFLAGS_DEBUG)
else ifeq ($(MODE), RELEASE)
	CFLAGS += $(CFLAGS_RELEASE)
else
	$(error Invalid MODE: $(MODE). Use DEBUG or RELEASE.)
endif

INCLUDE_DIRS := include external/stb/include external/BLAKE3/c external/pcre2/build/interface
INCLUDE_FLAGS := $(patsubst %,-I%, $(INCLUDE_DIRS))
LIBS_FILES := external/BLAKE3/c/build/libblake3.a external/pcre2/build/libpcre2-8.a build/stb_ds.o
LIB_FLAGS := -lsqlite3 -Lexternal/BLAKE3/c/build -lblake3 -Lexternal/pcre2/build -lpcre2-8 build/stb_ds.o
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/ftag
SRCS := $(wildcard src/*.c) $(wildcard src/**/*.c)
OBJS := $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(patsubst src/%.c, $(BUILD_DIR)/%.d, $(SRCS))

.PHONY: all clean-build clean-exe clean-compile-commands clean
.PRECIOUS: $(BUILD_DIR) $(BUILD_DIR)/%

all: $(TARGET)

define define_mkdir_target
$(1):
	@mkdir -p $(1)
endef
DIRS := $(sort $(dir $(OBJS)))

$(foreach dir,$(sort $(dir $(OBJS))),$(eval $(call define_mkdir_target,$(dir))))

-include $(DEPS)

external/BLAKE3/c/build/libblake3.a:
	cmake -S external/BLAKE3/c -B external/BLAKE3/c/build
	cmake --build external/BLAKE3/c/build

external/pcre2/build/libpcre2-8.a:
	cmake -S external/pcre2 -B external/pcre2/build -DPCRE2_SUPPORT_JIT=ON
	cmake --build external/pcre2/build

build/stb_ds.o: $(DIRS)
	$(CC) -c external/stb/impl/stb_ds.c -Iexternal/stb/include -o $@

$(BUILD_DIR)/%.o: src/%.c Makefile | $(DIRS)
	$(CC) $(CFLAGS) -MMD -MP $(INCLUDE_FLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(LIBS_FILES)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIB_FLAGS)

compile_commands.json: clean
	bear -- make

clean-build:
	rm -rf $(BUILD_DIR)

clean-exe: 
	rm -f $(TARGET)

clean-compile-commands:
	rm -f compile_commands.json

clean: clean-build clean-exe clean-compile-commands
