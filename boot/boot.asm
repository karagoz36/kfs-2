; boot.asm — the kernel's assembly entry point (NASM, 32-bit).
;
; It does two things:
;   1) place a multiboot header so GRUB recognises our kernel,
;   2) set up a stack and call kernel_main so the C code can run.

bits 32

; --- Multiboot v1 header constants ---
; GRUB scans the first 8 KB of the file, 4-byte aligned, for MAGIC.
; FLAGS = 0: we ask GRUB for nothing extra (no modules are loaded and the
; multiboot info struct is never read). Loading the kernel from its ELF
; headers is all we need.
MB_MAGIC    equ 0x1BADB002               ; multiboot 1 signature
MB_FLAGS    equ 0
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)   ; magic + flags + checksum must be 0

; The header lives in its own section; the linker script places that section at
; the very beginning of the file, so GRUB is guaranteed to find it.
section .multiboot
align 4
	dd MB_MAGIC
	dd MB_FLAGS
	dd MB_CHECKSUM

; --- Stack ---
; The multiboot spec does not guarantee the value of esp, so the stack left by
; GRUB cannot be trusted. C code cannot run without a stack, so we reserve
; 16 KB in .bss (which does not grow the binary).
; The x86 System V ABI requires the stack to be 16-byte aligned.
section .bss
align 16
stack_bottom:
	resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
	; The stack grows downwards on x86, so esp points at the top.
	mov esp, stack_top

	; Hand over to the C side. This call is not expected to return.
	call kernel_main

	; Should it return anyway: disable interrupts and halt forever.
	cli
.hang:
	hlt
	jmp .hang

; An empty section telling modern linkers that the stack need not be
; executable; without it ld emits a warning.
section .note.GNU-stack noalloc noexec nowrite progbits
