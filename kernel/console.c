/*
** console.c — virtual screen management, writing, scrolling and the cursor.
**
** Design: every virtual screen is a struct. Writes always go to that screen's
** own buffer; the screen currently displayed is additionally written into the
** VGA framebuffer. Background screens therefore keep their contents and a
** single blit is enough when switching.
*/

#include "console.h"
#include "string.h"

typedef struct s_console
{
	uint16_t buffer[VGA_WIDTH * VGA_HEIGHT]; /* the screen's own copy */
	size_t   row;                            /* cursor row */
	size_t   col;                            /* cursor column */
	uint8_t  color;                          /* current color attribute */
}	t_console;

static t_console g_consoles[CONSOLE_COUNT];
static size_t    g_active = 0;

/* Shorthand for the active screen. */
static t_console *current(void)
{
	return (&g_consoles[g_active]);
}

/*
** Every write goes through here, which is what makes several screens possible:
** the buffer is always updated, the framebuffer only when this screen is the
** one being displayed. Background screens therefore keep accumulating text.
*/
static void put_cell(t_console *c, size_t index, uint16_t cell)
{
	c->buffer[index] = cell;
	if (c == current())
		vga_write_cell(index, cell);
}

/*
** There are two cursors: ours (row/col in the struct) and the hardware one
** blinking on screen. Writing a character only moves ours, so this is called
** after every operation that changes the position to keep them in step.
*/
static void sync_cursor(void)
{
	vga_move_cursor(current()->row, current()->col);
}

/*
** Scroll (bonus): when the screen is full, shift every line up by one, blank
** the last line and keep the cursor on it.
*/
static void scroll(t_console *c)
{
	size_t i;

	/*
	** Destination is the first line, source is the second (buffer + one row),
	** length is 24 rows. The first line is overwritten and everything below
	** moves up by one.
	*/
	k_memcpy(c->buffer, c->buffer + VGA_WIDTH,
		(VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(uint16_t));

	/* Fill the last line with spaces */
	i = (VGA_HEIGHT - 1) * VGA_WIDTH;
	while (i < VGA_HEIGHT * VGA_WIDTH)
		c->buffer[i++] = vga_entry(' ', c->color);

	c->row = VGA_HEIGHT - 1;
	if (c == current())
		vga_blit(c->buffer);
}

/* Move to the next line; scroll when the bottom of the screen is reached. */
static void newline(t_console *c)
{
	c->col = 0;
	c->row++;
	if (c->row >= VGA_HEIGHT)
		scroll(c);
}

/* Resets every screen and makes the first one active. */
void console_init(void)
{
	size_t i = 0;

	while (i < CONSOLE_COUNT)
	{
		g_consoles[i].row = 0;
		g_consoles[i].col = 0;
		g_consoles[i].color = vga_color(VGA_LIGHT_GREY, VGA_BLACK);
		g_active = i;             /* console_clear works on the active screen */
		console_clear();
		i++;
	}
	g_active = 0;
	vga_enable_cursor();
	vga_blit(g_consoles[0].buffer);
	sync_cursor();
}

/* Changes the active screen and paints it on the display (bonus). */
void console_switch(size_t index)
{
	if (index >= CONSOLE_COUNT || index == g_active)
		return ;
	g_active = index;
	vga_blit(current()->buffer);
	sync_cursor();
}

/* Sets the color used by the characters written from now on (bonus). */
void console_set_color(uint8_t fg, uint8_t bg)
{
	current()->color = vga_color(fg, bg);
}

/* Fills the active screen with spaces and moves the cursor back to the top. */
void console_clear(void)
{
	t_console *c = current();
	size_t     i = 0;

	/*
	** Spaces carrying the console's own color, rather than plain zeros: every
	** cell then holds a defined attribute, so scrolling and backspace (which
	** write spaces too) blend in with the rest of the screen.
	*/
	while (i < VGA_WIDTH * VGA_HEIGHT)
	{
		c->buffer[i] = vga_entry(' ', c->color);
		i++;
	}
	c->row = 0;
	c->col = 0;
	vga_blit(c->buffer);
	sync_cursor();
}

/*
** Writes one character. Special characters:
**  '\n' -> new line, '\t' -> align to the next multiple of 4,
**  '\b' -> erase (backspace).
*/
void console_putchar(char c)
{
	t_console *con = current();

	if (c == '\n')
		newline(con);
	else if (c == '\t')
	{
		size_t spaces = 4 - (con->col % 4);

		while (spaces-- > 0)
			console_putchar(' ');
	}
	else if (c == '\b')
	{
		/*
		** Step back one cell; at the start of a line go up to the previous
		** one. The row check matters: col is unsigned, so decrementing it at
		** column 0 would wrap around to a huge value.
		*/
		if (con->col > 0)
			con->col--;
		else if (con->row > 0)
		{
			con->row--;
			con->col = VGA_WIDTH - 1;
		}
		put_cell(con, con->row * VGA_WIDTH + con->col,
			vga_entry(' ', con->color));
	}
	else
	{
		put_cell(con, con->row * VGA_WIDTH + con->col,
			vga_entry(c, con->color));
		con->col++;
		if (con->col >= VGA_WIDTH)  /* line overflowed -> wrap automatically */
			newline(con);
	}
	sync_cursor();
}

/* Writes a null-terminated string. */
void console_write(const char *s)
{
	size_t i = 0;

	while (s[i] != '\0')
		console_putchar(s[i++]);
}
