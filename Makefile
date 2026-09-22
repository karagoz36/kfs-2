# ==============================================================================
# KFS-2 Makefile
#
# Two languages are compiled: NASM (boot code) and C (the kernel).
# All object files are then linked with our own linker script into a multiboot
# compliant ELF binary, which GRUB turns into a bootable ISO.
#
# On Linux (Fedora/Debian):          make && make run
# On macOS (no native toolchain):    make docker  -> builds the ISO, then make run
# ==============================================================================

NAME      := kernel.bin
ISO       := kfs.iso

BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/isodir

CC := gcc
AS := nasm
LD := ld

# Sources
ASM_SRCS := boot/boot.asm \
            boot/gdt_flush.asm
C_SRCS   := kernel/main.c \
            kernel/console.c \
            kernel/printk.c \
            kernel/gdt.c \
            kernel/stack.c \
            kernel/shell.c \
            drivers/vga.c \
            drivers/keyboard.c \
            lib/string.c

ASM_OBJS := $(ASM_SRCS:%.asm=$(BUILD_DIR)/%.o)
C_OBJS   := $(C_SRCS:%.c=$(BUILD_DIR)/%.o)
OBJS     := $(ASM_OBJS) $(C_OBJS)
DEPS     := $(C_OBJS:.o=.d)

# ------------------------------------------------------------------------------
# Flags
#   -m32                 : emit i386 (32-bit) code; the subject mandates x86
#   -ffreestanding       : no standard library or runtime, main is not special
#   -fno-builtin         : do not let gcc turn our loops into memcpy/strlen calls
#   -fno-stack-protector : the stack canary comes from libc, which we do not have
#   -nostdlib/-nodefaultlibs : never link host libraries (the kernel would not boot)
#   -fno-pie / -fno-pic  : fixed-address code running at 1 MB, no relocation
#   -fno-omit-frame-pointer : keep ebp as a frame pointer so the stack
#                          printer can walk the call chain (KFS-2)
#   -mgeneral-regs-only  : no SSE/MMX/x87. On an x86-64 host, -m32 still
#                          enables SSE2 and gcc vectorises loops with xmm
#                          registers; SSE is disabled at boot (CR4.OSFXSR=0)
#                          so the first such instruction is an invalid opcode
# Note: the subject's -fno-exception and -fno-rtti are C++ flags with no C
#       equivalent (gcc warns about them), so they were adapted to the language.
# ------------------------------------------------------------------------------
CFLAGS := -m32 -std=gnu99 -O2 \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -fno-pie -fno-pic -nostdlib -nodefaultlibs \
          -fno-omit-frame-pointer -mgeneral-regs-only \
          -Wall -Wextra -Werror \
          -Iinclude -MMD -MP

ASFLAGS := -f elf32

# Our own linker script is used instead of the host's .ld file.
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

# ------------------------------------------------------------------------------
# Tool names differ per distribution (grub2-* on Fedora, grub-* on Debian)
# ------------------------------------------------------------------------------
GRUB_MKRESCUE := $(shell command -v grub-mkrescue 2>/dev/null || command -v grub2-mkrescue 2>/dev/null)
GRUB_FILE     := $(shell command -v grub-file 2>/dev/null || command -v grub2-file 2>/dev/null)
GRUB_I386_DIR := $(firstword $(wildcard /usr/lib/grub/i386-pc /usr/lib/grub2/i386-pc /usr/share/grub2/i386-pc))

# Keep the ISO small: BIOS (i386-pc) target only, no fonts/locales/themes and
# only the GRUB modules we actually need. The part_* modules are not needed to
# boot, but GRUB tries to load every partition-map module listed in its index
# at startup and prints "part_xxx.mod not found" for each missing one.
GRUB_MODULES := multiboot normal biosdisk iso9660 \
                part_acorn part_amiga part_apple part_bsd part_dfly part_dvh \
                part_gpt part_msdos part_plan part_sun part_sunpc
GRUB_FLAGS := --fonts= --locales= --themes= --compress=xz \
              --install-modules="$(GRUB_MODULES)"
ifneq ($(GRUB_I386_DIR),)
GRUB_FLAGS += -d $(GRUB_I386_DIR)
endif

# QEMU: -boot d makes the BIOS try the CD-ROM first (by default it tries the
# hard disk and the floppy before, which fail and slow the boot down).
# On Linux, KVM=1 enables hardware acceleration.
QEMU  := qemu-system-i386
QEMUFLAGS := -cdrom $(ISO) -boot d -m 64
ifeq ($(KVM),1)
QEMUFLAGS += -enable-kvm
endif

# macOS has no native toolchain, so the build can run inside a container
DOCKER_IMAGE := kfs2-build
DOCKER_RUN   := docker run --rm --platform linux/amd64 -v "$(PWD)":/kfs -w /kfs

# grub-file only exists on Linux; on macOS the multiboot check runs in the container
ifneq ($(GRUB_FILE),)
CHECK_MULTIBOOT := $(GRUB_FILE)
else
CHECK_MULTIBOOT := $(DOCKER_RUN) $(DOCKER_IMAGE) grub-file
endif

.PHONY: all iso run clean fclean re check docker docker-image docker-shell

all: $(ISO)

# --- Compilation rules ---
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# --- Link: every object into a single multiboot ELF ---
$(ISO_DIR)/boot/$(NAME): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "==> linked $(NAME)"

# --- ISO: puts GRUB and the kernel together into a bootable image ---
$(ISO): $(ISO_DIR)/boot/$(NAME) grub/grub.cfg
	@if [ -z "$(GRUB_MKRESCUE)" ]; then \
		echo "ERROR: grub-mkrescue not found (Fedora: grub2-tools-extra, macOS: use 'make docker')."; \
		exit 1; \
	fi
	@mkdir -p $(ISO_DIR)/boot/grub
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) $(GRUB_FLAGS) -o $(ISO) $(ISO_DIR)
	@echo "==> $(ISO) ready ($$(du -h $(ISO) | cut -f1))"

iso: $(ISO)

# Is the kernel really multiboot compliant and the ISO within the 10 MB limit?
check: $(ISO)
	$(CHECK_MULTIBOOT) --is-x86-multiboot $(ISO_DIR)/boot/$(NAME) && echo "multiboot: OK"
	@test $$(stat -c %s $(ISO) 2>/dev/null || stat -f %z $(ISO)) -lt 10485760 \
		&& echo "size: under 10 MB OK"

run: $(ISO)
	$(QEMU) $(QEMUFLAGS)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(ISO)

re: fclean all

# --- macOS: build inside a Linux container ---
docker-image:
	docker build --platform linux/amd64 -t $(DOCKER_IMAGE) .

docker: docker-image
	$(DOCKER_RUN) $(DOCKER_IMAGE) make re

docker-shell: docker-image
	$(DOCKER_RUN) -it $(DOCKER_IMAGE) bash

# Header dependencies (generated by -MMD): a changed header rebuilds its .c
-include $(DEPS)
