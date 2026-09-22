/*
** io.h — x86 port I/O helpers.
**
** Some devices (the VGA cursor registers, the PS/2 keyboard controller) are
** not reached through memory but through a separate I/O address space. That
** space is only accessible with the 'in' and 'out' instructions, which have no
** equivalent in C, so inline assembly is required.
*/

#ifndef IO_H
#define IO_H

#include "types.h"

/* Writes one byte to the given port. "a" = al register, "Nd" = dx or immediate */
static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Reads one byte from the given port. */
static inline uint8_t inb(uint16_t port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return (value);
}

#endif
