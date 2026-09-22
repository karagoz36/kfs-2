/*
** main.c — the kernel's entry point on the C side.
**
** Flow: GRUB -> boot.asm (_start) -> kernel_main().
** By the time we get here the CPU is in 32-bit protected mode, interrupts are
** disabled and boot.asm has set up a stack for us.
*/

#include "console.h"
#include "printk.h"
#include "keyboard.h"
#include "gdt.h"
#include "shell.h"

/* Header, banner and shell prompt of one virtual screen. */
static void draw_screen(size_t index)
{
	console_set_color(VGA_BLACK, VGA_LIGHT_GREY);
	printk(" KFS-2  screen %u/%u ", (uint32_t)(index + 1),
		(uint32_t)CONSOLE_COUNT);
	console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
	printk("\n\n42\n\n");
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	printk("Kernel From Scratch 2 - GDT loaded at %p, kernel stack ready.\n",
		(void *)gdt_table);
	console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
	printk("Type 'help' for the commands. Switch screen: Alt+1..%d.\n\n",
		CONSOLE_COUNT);
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	shell_init();
}

/* Every virtual screen (bonus) gets the same content, then screen 1 is shown. */
static void draw_screens(void)
{
	size_t i = 0;

	while (i < CONSOLE_COUNT)
	{
		console_switch(i);
		draw_screen(i);
		i++;
	}
	console_switch(0);
}

/*
** kernel_main never returns: the bootloader is gone, there is nowhere to
** return to. The main loop polls the keyboard and feeds the shell.
**
** Why there is no 'hlt': GRUB leaves interrupts disabled (IF = 0) and there
** is no IDT yet, so we cannot issue 'sti'. A CPU executing 'hlt' with IF = 0
** can never be woken by an interrupt and the machine freezes there. Hence the
** busy-polling loop.
*/
void kernel_main(void)
{
	char c;

	/* First thing: replace GRUB's temporary GDT with ours (mandatory part) */
	gdt_init();

	console_init();
	draw_screens();

	while (TRUE)
	{
		c = keyboard_poll();
		if (c != 0)
			shell_input(c);
	}
}
