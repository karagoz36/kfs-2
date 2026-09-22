/*
** linker.ld — the linker script describing the kernel's memory layout.
**
** The host system's own .ld file cannot be used (the subject forbids it and it
** would not produce a bootable binary anyway), so the layout is written here.
*/

/* The entry point the bootloader jumps to (defined in boot.asm) */
ENTRY(_start)

SECTIONS
{
	/*
	** The GDT (KFS-2) must sit at physical address 0x800. That is below the
	** 1 MB GRUB loads us at, so it cannot be a real section: the symbol just
	** names the address and kernel/gdt.c fills the table at run time.
	*/
	gdt_table = 0x00000800;

	/*
	** The kernel is placed at 1 MB: the first megabyte holds the BIOS, the
	** VGA framebuffer and other device regions, which are not safe to write.
	*/
	. = 1M;

	/* The multiboot header must sit within the first 8 KB -> put it first */
	.text ALIGN(4K) :
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data (string literals, keymap tables) */
	.rodata ALIGN(4K) :
	{
		*(.rodata)
	}

	/* Initialised global/static variables */
	.data ALIGN(4K) :
	{
		*(.data)
	}

	/*
	** Zeroed variables and the stack. They take no space in the binary and
	** are only reserved in memory at run time.
	*/
	.bss ALIGN(4K) :
	{
		*(COMMON)
		*(.bss)
	}
}
