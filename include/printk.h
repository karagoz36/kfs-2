/*
** printk.h — simple formatted printing for the kernel (bonus).
** Supported format specifiers: %c %s %d %i %u %x %p %%
*/

#ifndef PRINTK_H
#define PRINTK_H

void printk(const char *format, ...);

#endif
