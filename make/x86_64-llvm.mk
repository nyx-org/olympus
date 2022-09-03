CC = clang
LD = ld.lld

CFLAGS += $(BASE_CFLAGS) -target x86_64-unknown-elf -fno-stack-check \
        -fno-stack-protector \
        -fno-pic \
        -fno-pie
 

LINK_FLAGS = -nostdlib -melf_x86_64 -static