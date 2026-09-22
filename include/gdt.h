/*
** gdt.h — the Global Descriptor Table.
**
** The GDT tells the CPU which memory "segments" exist: their base address,
** their size and who may use them (kernel or user, code or data). The subject
** asks for six segments, plus the null entry the CPU requires at index 0, and
** for the table to live at physical address 0x800.
*/

#ifndef GDT_H
#define GDT_H

#include "types.h"

#define GDT_ENTRIES 7

/*
** One 8-byte descriptor, in the exact layout the CPU expects. The base and the
** limit are scattered across the structure for historical (80286) reasons.
** 'packed' forbids the compiler from inserting padding between the fields.
*/
typedef struct s_gdt_entry
{
	uint16_t limit_low;    /* limit bits 0..15 */
	uint16_t base_low;     /* base bits 0..15 */
	uint8_t  base_mid;     /* base bits 16..23 */
	uint8_t  access;       /* present, privilege level, code/data, read/write */
	uint8_t  granularity;  /* limit bits 16..19 (low nibble) + flags (high nibble) */
	uint8_t  base_high;    /* base bits 24..31 */
}	__attribute__((packed)) t_gdt_entry;

/* The table itself: 7 descriptors at 0x800. The address comes from linker.ld */
extern t_gdt_entry gdt_table[GDT_ENTRIES];

void gdt_init(void);
void gdt_print(void);

#endif
