MODE ?= DEBUG
CC ?= clang

PREFIX ?= /usr/local
BIN_DIR ?= $(PREFIX)/bin
ZSH_COMPLETIONS_DIR ?= $(PREFIX)/share/zsh/site-functions

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m644

CFLAGS := -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wimplicit-fallthrough -std=c2x
CFLAGS_DEBUG := -g -O0
CFLAGS_RELEASE := -O3

ifdef VERSION
	CFLAGS += -DFTAG_VERSION=\"$(VERSION)\"
endif

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

.PHONY: all install uninstall clean-build clean-exe clean-compile-commands clean FORCE
.PRECIOUS: $(BUILD_DIR) $(BUILD_DIR)/%

all: $(TARGET)

define define_mkdir_target
$(1):
	@mkdir -p $(1)
endef
DIRS := $(sort $(dir $(OBJS)))

$(foreach dir,$(sort $(dir $(OBJS))),$(eval $(call define_mkdir_target,$(dir))))

-include $(DEPS)

external/BLAKE3/c/build/libblake3.a: FORCE
	cmake -S external/BLAKE3/c -B external/BLAKE3/c/build
	cmake --build external/BLAKE3/c/build --parallel

external/pcre2/build/libpcre2-8.a: FORCE
	cmake -S external/pcre2 -B external/pcre2/build -DPCRE2_SUPPORT_JIT=ON
	cmake --build external/pcre2/build --parallel

build/stb_ds.o: | $(DIRS)
	$(CC) -O3 -c external/stb/impl/stb_ds.c -Iexternal/stb/include -o $@

$(BUILD_DIR)/%.o: src/%.c Makefile | $(DIRS) $(LIBS_FILES)
	$(CC) $(CFLAGS) -MMD -MP $(INCLUDE_FLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(LIBS_FILES)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIB_FLAGS)

compile_commands.json: clean
	bear -- make

install: $(TARGET)
	$(INSTALL) -d $(DESTDIR)$(BIN_DIR) $(DESTDIR)$(ZSH_COMPLETIONS_DIR)
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(BIN_DIR)/ftag
	$(INSTALL_DATA) completions/_ftag $(DESTDIR)$(ZSH_COMPLETIONS_DIR)/_ftag

uninstall:
	rm -f $(DESTDIR)$(BIN_DIR)/ftag $(DESTDIR)$(ZSH_COMPLETIONS_DIR)/_ftag

clean-build:
	rm -rf $(BUILD_DIR)
	rm -rf external/BLAKE3/c/build
	rm -rf external/pcre2/build

clean-exe: 
	rm -f $(TARGET)

clean-compile-commands:
	rm -f compile_commands.json

clean: clean-build clean-exe clean-compile-commands
