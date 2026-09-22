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
#include "string.h"

/* Writes a short header on every virtual screen (bonus: multiple screens). */
static void draw_headers(void)
{
	size_t i = 0;

	while (i < CONSOLE_COUNT)
	{
		console_switch(i);
		console_set_color(VGA_BLACK, VGA_LIGHT_GREY);
		printk(" KFS-1  screen %u/%u ", (uint32_t)(i + 1),
			(uint32_t)CONSOLE_COUNT);
		console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
		printk("\n\n");
		i++;
	}
	console_switch(0);
}

/* The mandatory part: display "42" on the screen. */
static void print_banner(void)
{
	console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
	printk("42\n\n");
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/* A short summary showing the bonus features and how to use them. */
static void print_info(void)
{
	printk("Kernel From Scratch 1 - bootloader: GRUB (multiboot)\n");
	printk("printk test: %s | %d | %u | 0x%x | %p | %c | %%\n",
		"string", -42, 42u, 48879u, (void *)0xB8000, 'K');
	printk("k_strlen(\"42\") = %d, k_strcmp(\"a\", \"a\") = %d\n",
		(int)k_strlen("42"), k_strcmp("a", "a"));
	console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
	printk("\nKeyboard is live: type away (backspace works).\n");
	printk("Switch screen: Alt+1..%d (Ctrl+1..%d works too).\n\n",
		CONSOLE_COUNT, CONSOLE_COUNT);
	console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/*
** kernel_main never returns: the bootloader is gone, there is nowhere to
** return to. The main loop simply polls the keyboard.
**
** Why there is no 'hlt': GRUB leaves interrupts disabled (IF = 0) and KFS_1
** has no IDT yet, so we cannot issue 'sti'. A CPU executing 'hlt' with IF = 0
** can never be woken by an interrupt and the machine freezes there. Hence the
** busy-polling loop.
*/
void kernel_main(void)
{
	console_init();
	draw_headers();
	print_banner();
	print_info();

	while (TRUE)
		keyboard_poll();
}
