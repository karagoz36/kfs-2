/*
** vga.h — low-level interface to the VGA text mode (80x25) hardware.
**
** BIOS/GRUB leave us in text mode. In that mode the screen is a framebuffer
** starting at 0xB8000: every cell is 2 bytes, the low byte holds the ASCII
** character and the high byte the color attribute.
*/

#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
/*
** The framebuffer is addressed as an array of 16-bit cells, so index
** row * VGA_WIDTH + col selects a character. 'volatile' stops the compiler
** from optimising away writes it considers useless: this is a device, not RAM.
*/
#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)

/* The standard 16-color VGA palette (bonus: color support) */
enum e_vga_color
{
	VGA_BLACK = 0,
	VGA_BLUE,
	VGA_GREEN,
	VGA_CYAN,
	VGA_RED,
	VGA_MAGENTA,
	VGA_BROWN,
	VGA_LIGHT_GREY,
	VGA_DARK_GREY,
	VGA_LIGHT_BLUE,
	VGA_LIGHT_GREEN,
	VGA_LIGHT_CYAN,
	VGA_LIGHT_RED,
	VGA_LIGHT_MAGENTA,
	VGA_YELLOW,
	VGA_WHITE
};

/*
** Background (high 4 bits) + foreground (low 4 bits) -> attribute byte.
** Shifting bg left by 4 moves it into the upper nibble, then OR merges the two:
**   fg = 11 (light cyan), bg = 0 (black)  ->  0x0B
*/
static inline uint8_t vga_color(uint8_t fg, uint8_t bg)
{
	return ((uint8_t)(fg | (bg << 4)));
}

/*
** Character + color -> the 16-bit cell written to the framebuffer.
** The character stays in the low byte, the attribute is shifted into the high
** byte:  '4' (0x34) with attribute 0x0B  ->  0x0B34
*/
static inline uint16_t vga_entry(char c, uint8_t color)
{
	return ((uint16_t)(uint8_t)c | ((uint16_t)color << 8));
}

void vga_write_cell(size_t index, uint16_t cell);
void vga_blit(const uint16_t *buffer);
void vga_enable_cursor(void);
void vga_move_cursor(size_t row, size_t col);

#endif
