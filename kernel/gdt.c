/*
** gdt.c — building the GDT at 0x800 and handing it to the CPU.
**
** Why at run time and not through the linker: GRUB refuses to load ELF
** segments below 1 MB, so the table cannot be part of the binary. Instead the
** seven descriptors are written to 0x800 by hand during boot.
**
** Every segment is "flat": base 0 and limit 4 GB. Segmentation is therefore
** not used to separate memory (paging does that in later subjects); the table
** exists because protected mode cannot run without one, and because the
** privilege levels (ring 0 vs ring 3) are stored here.
**
** A segment selector is the byte offset of its entry: index * 8. So kernel
** code is 0x08, kernel data 0x10, kernel stack 0x18, user code 0x20, user
** data 0x28 and user stack 0x30. gdt_flush.asm loads the first three.
*/

#include "gdt.h"
#include "printk.h"

/* Bits of the access byte */
#define ACC_PRESENT 0x80  /* the segment exists */
#define ACC_RING0   0x00  /* kernel privilege */
#define ACC_RING3   0x60  /* user privilege */
#define ACC_SEGMENT 0x10  /* code or data segment (not a system descriptor) */
#define ACC_EXEC    0x08  /* code segment */
#define ACC_RW      0x02  /* code: readable, data: writable */

#define ACCESS_CODE(ring) (ACC_PRESENT | (ring) | ACC_SEGMENT | ACC_EXEC | ACC_RW)
#define ACCESS_DATA(ring) (ACC_PRESENT | (ring) | ACC_SEGMENT | ACC_RW)

/* Flags nibble: G = 1 (limit counted in 4 KB pages), D/B = 1 (32-bit segment) */
#define FLAGS_32BIT_4K 0xC

/* 0xFFFFF pages of 4 KB = 4 GB: the whole address space */
#define LIMIT_4GB 0xFFFFF

/* What the 'lgdt' instruction reads: the table's size minus one and its address */
typedef struct s_gdt_ptr
{
	uint16_t limit;
	uint32_t base;
}	__attribute__((packed)) t_gdt_ptr;

/*
** The stack segments use the same access byte as data. The "expand-down" bit
** (0x04) exists for stacks, but with a full 4 GB limit it would make every
** offset invalid on real hardware (QEMU does not check limits, which hides
** the mistake). A plain writable segment is correct and simpler.
*/
static const struct s_segment
{
	const char *name;
	uint8_t     access;
}	g_segments[GDT_ENTRIES] = {
	{"null        ", 0},
	{"kernel code ", ACCESS_CODE(ACC_RING0)},  /* 0x9A */
	{"kernel data ", ACCESS_DATA(ACC_RING0)},  /* 0x92 */
	{"kernel stack", ACCESS_DATA(ACC_RING0)},  /* 0x92 */
	{"user code   ", ACCESS_CODE(ACC_RING3)},  /* 0xFA */
	{"user data   ", ACCESS_DATA(ACC_RING3)},  /* 0xF2 */
	{"user stack  ", ACCESS_DATA(ACC_RING3)},  /* 0xF2 */
};

/* Defined in boot/gdt_flush.asm: runs lgdt and reloads the segment registers */
extern void gdt_flush(const t_gdt_ptr *ptr);

/* Splits base and limit into the scattered fields of a descriptor. */
static void gdt_set_entry(size_t i, uint32_t base, uint32_t limit,
	uint8_t access, uint8_t flags)
{
	gdt_table[i].limit_low   = (uint16_t)(limit & 0xFFFF);
	gdt_table[i].base_low    = (uint16_t)(base & 0xFFFF);
	gdt_table[i].base_mid    = (uint8_t)((base >> 16) & 0xFF);
	gdt_table[i].access      = access;
	gdt_table[i].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (flags << 4));
	gdt_table[i].base_high   = (uint8_t)((base >> 24) & 0xFF);
}

/*
** Writes the seven descriptors at 0x800 and makes the CPU use them.
** Entry 0 must stay all zeros: the CPU uses selector 0 as "no segment".
*/
void gdt_init(void)
{
	t_gdt_ptr ptr;
	size_t    i;

	gdt_set_entry(0, 0, 0, 0, 0);
	i = 1;
	while (i < GDT_ENTRIES)
	{
		gdt_set_entry(i, 0, LIMIT_4GB, g_segments[i].access, FLAGS_32BIT_4K);
		i++;
	}
	/* lgdt copies these 6 bytes into the CPU, so a local is enough */
	ptr.limit = sizeof(gdt_table) - 1;
	ptr.base  = (uint32_t)gdt_table;
	gdt_flush(&ptr);
}

/*
** Reads the descriptors back from 0x800 and prints them in a readable form.
** Note: the access byte of the segments in use shows 0x93 instead of 0x92:
** bit 0 is the "accessed" flag, which the CPU sets itself when a segment
** register is loaded with that selector.
*/
void gdt_print(void)
{
	const t_gdt_entry *e;
	uint32_t           base;
	uint32_t           limit;
	size_t             i = 0;

	printk("GDT at %p (%u entries):\n", (void *)gdt_table, (uint32_t)GDT_ENTRIES);
	printk("  sel   segment       base        limit    access  flags\n");
	while (i < GDT_ENTRIES)
	{
		e = &gdt_table[i];
		base  = e->base_low | ((uint32_t)e->base_mid << 16)
			| ((uint32_t)e->base_high << 24);
		limit = e->limit_low | ((uint32_t)(e->granularity & 0x0F) << 16);
		printk("  0x%02x  %s  0x%08x  0x%05x  0x%02x    0x%x\n",
			(uint32_t)(i * 8), g_segments[i].name, base, limit,
			(uint32_t)e->access, (uint32_t)(e->granularity >> 4));
		i++;
	}
}
