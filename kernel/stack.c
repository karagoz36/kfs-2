/*
** stack.c — dump of the kernel stack and of the call frames.
**
** The stack is the 16 KB reserved in boot.asm between stack_bottom and
** stack_top. It grows downwards: esp points at the most recent value pushed,
** and everything from esp up to stack_top is in use.
**
** Two views are printed:
**  1) a hexdump of the bytes in use, 16 per line, like 'x/4wx' in gdb;
**  2) the chain of call frames. Every function starts by saving ebp and
**     pointing it at the saved copy (-fno-omit-frame-pointer), so each frame
**     holds [saved ebp][return address] and following ebp walks the callers.
*/

#include "stack.h"
#include "console.h"
#include "printk.h"

extern uint8_t stack_bottom[];
extern uint8_t stack_top[];

#define BYTES_PER_LINE 16
#define MAX_DUMP_LINES 12  /* keeps the whole dump on one 25-line screen */
#define MAX_FRAMES     6

/* One line: address, four 32-bit words, then the same bytes as ASCII. */
static void dump_line(uint32_t addr)
{
	const uint32_t *words = (const uint32_t *)addr;
	const uint8_t  *bytes = (const uint8_t *)addr;
	size_t          i;

	printk("%08x:  ", addr);
	i = 0;
	while (i < BYTES_PER_LINE / sizeof(uint32_t))
		printk("%08x ", words[i++]);
	printk(" |");
	i = 0;
	while (i < BYTES_PER_LINE)
	{
		console_putchar(bytes[i] >= ' ' && bytes[i] <= '~' ? (char)bytes[i] : '.');
		i++;
	}
	printk("|\n");
}

/* Follows the saved-ebp chain and prints the return address of each frame. */
static void print_frames(const uint32_t *frame)
{
	int depth = 0;

	printk("call frames:\n");
	while ((uint8_t *)frame >= stack_bottom && (uint8_t *)frame + 8 <= stack_top
		&& depth < MAX_FRAMES)
	{
		printk("  #%d  ebp=%p  return to %p\n", depth, (void *)frame,
			(void *)frame[1]);
		frame = (const uint32_t *)frame[0];
		depth++;
	}
}

void print_kernel_stack(void)
{
	uint32_t        esp;
	const uint32_t *ebp = __builtin_frame_address(0);  /* this function's ebp */
	uint32_t        top = (uint32_t)stack_top;
	uint32_t        addr;
	int             lines = 0;

	__asm__ volatile("mov %%esp, %0" : "=r"(esp));

	printk("kernel stack: %p - %p (%u bytes), %u in use\n",
		(void *)stack_bottom, (void *)stack_top,
		top - (uint32_t)stack_bottom, top - esp);
	printk("esp=%p  ebp=%p\n", (void *)esp, (void *)ebp);

	/* Start on a 16-byte boundary so the addresses of the lines are round */
	addr = esp & ~(uint32_t)(BYTES_PER_LINE - 1);
	while (addr < top && lines < MAX_DUMP_LINES)
	{
		dump_line(addr);
		addr += BYTES_PER_LINE;
		lines++;
	}
	if (addr < top)
		printk("... %u more bytes up to %p\n", top - addr, (void *)stack_top);
	print_frames(ebp);
}
