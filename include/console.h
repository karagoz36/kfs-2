/*
** console.h — interface between the kernel and the screen.
**
** We keep several "virtual screens" (bonus): each one has its own 80x25
** buffer, its own cursor position and its own color. Only the active screen
** is mirrored (blitted) into the VGA framebuffer.
*/

#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"
#include "vga.h"

#define CONSOLE_COUNT 4

void   console_init(void);
void   console_switch(size_t index);
void   console_set_color(uint8_t fg, uint8_t bg);
void   console_clear(void);
void   console_putchar(char c);
void   console_write(const char *s);

#endif
