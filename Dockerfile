# Build environment used while developing on macOS (Apple Silicon).
# macOS has no gcc -m32, no ld -m elf_i386 and no grub-mkrescue; this container
# provides the same tools inside Linux/x86_64 as the Fedora machines at school.
#
# Usage:  make docker   (the Makefile builds this image and runs 'make re')

FROM debian:bookworm-slim

# Note: -o Acquire::Check-Date=false prevents apt's "Release file is not valid
# yet" error when the container and host clocks differ.
RUN apt-get -o Acquire::Check-Date=false update && \
	apt-get install -y --no-install-recommends \
	build-essential \
	gcc-multilib \
	nasm \
	binutils \
	make \
	grub-common \
	grub-pc-bin \
	xorriso \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /kfs
