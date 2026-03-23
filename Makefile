MODE ?= DEBUG
CC ?= clang

CFLAGS := -Wall -Wextra -Werror -std=c23
CFLAGS_DEBUG := -g -O0
CFLAGS_RELEASE := -O3

ifeq ($(MODE), DEBUG)
	CFLAGS += $(CFLAGS_DEBUG)
else ifeq ($(MODE), RELEASE)
	CFLAGS += $(CFLAGS_RELEASE)
else
	$(error Invalid MODE: $(MODE). Use DEBUG or RELEASE.)
endif

INCLUDE_DIRS := include
INCLUDE_FLAGS := $(patsubst %,-I%, $(INCLUDE_DIRS))
LIBS_FILES :=
LIB_FLAGS := -lsqlite3
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
