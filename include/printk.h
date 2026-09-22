/*
** printk.h — simple formatted printing for the kernel (bonus).
** Supported format specifiers: %c %s %d %i %u %x %p %%
** %u, %x and %p accept a zero-padding width: %08x prints exactly 8 digits.
*/

#ifndef PRINTK_H
#define PRINTK_H

void printk(const char *format, ...);

#endif
