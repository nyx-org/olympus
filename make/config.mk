# TODO: make this configurable through kconfig

TOOLCHAIN = x86_64-llvm
BUILD_DIR = build
MKCWD = mkdir -p $(@D)
BASE_CFLAGS = -Wall -Wextra -Werror -std=c99 -Isrc

include make/$(TOOLCHAIN).mk