; gdt_flush.asm — loads the GDT register and reloads every segment register.
;
; Writing the table at 0x800 is not enough: the CPU only looks at it once
; 'lgdt' has been executed, and the segment registers still hold whatever
; GRUB left in them. They must be reloaded with our own selectors, which are
; the byte offsets of the entries in the table (index * 8, see kernel/gdt.c).
;
; C prototype: void gdt_flush(const t_gdt_ptr *ptr);

bits 32

KERNEL_CODE  equ 0x08   ; entry 1
KERNEL_DATA  equ 0x10   ; entry 2
KERNEL_STACK equ 0x18   ; entry 3

section .text
global gdt_flush

gdt_flush:
	mov eax, [esp + 4]      ; first argument: address of the 6-byte GDT pointer
	lgdt [eax]              ; "declare" the table to the CPU

	; CS cannot be written with 'mov'. A far jump reloads it with the kernel
	; code selector and continues at the label right below.
	jmp KERNEL_CODE:.reload_segments

.reload_segments:
	mov ax, KERNEL_DATA
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ax, KERNEL_STACK
	mov ss, ax
	ret

section .note.GNU-stack noalloc noexec nowrite progbits
