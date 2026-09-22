/*
** keyboard.c — PS/2 keyboard driver (bonus).
**
** KFS_1 has no IDT/interrupt support yet, so the keyboard is read by polling:
** we check the PS/2 controller's status port and, when its output buffer is
** full, read the scancode from the data port.
**
** The keyboard sends "scancode set 1": pressing a key produces a make code,
** releasing it produces the same code with 0x80 added (a break code).
*/

#include "keyboard.h"
#include "console.h"
#include "io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_OUTPUT_FULL 0x01  /* status register bit 0: data available */

#define SC_RELEASE_FLAG 0x80  /* the bit that marks break codes */
#define SC_EXTENDED     0xE0  /* some keys send two bytes */

#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_CTRL         0x1D
#define SC_ALT          0x38
#define SC_DIGIT1       0x02  /* 1..4 -> 0x02, 0x03, 0x04, 0x05 */

/* Current state of the modifier keys */
static bool_t g_shift = FALSE;
static bool_t g_ctrl  = FALSE;
static bool_t g_alt   = FALSE;

/* Whether the next scancode follows an 0xE0 (extended) prefix */
static bool_t g_extended = FALSE;

/*
** Scancode -> ASCII table (US QWERTY, set 1).
** 0 means the key has no printable equivalent (function keys, modifiers, ...)
*/
static const char g_keymap[SC_RELEASE_FLAG] = {
	0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	0,    '*', 0,   ' '
};

/* The characters the same scancodes produce while shift is held */
static const char g_keymap_shift[SC_RELEASE_FLAG] = {
	0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
	0,    '*', 0,   ' '
};

/* Handles modifier keys; returns TRUE when it did (no character is printed). */
static bool_t handle_modifier(uint8_t code, bool_t pressed)
{
	if (code == SC_LSHIFT || code == SC_RSHIFT)
		g_shift = pressed;
	else if (code == SC_CTRL)
		g_ctrl = pressed;
	else if (code == SC_ALT)
		g_alt = pressed;
	else
		return (FALSE);
	return (TRUE);
}

/*
** Screen switching shortcut (bonus): Alt + 1..4 (Ctrl + 1..4 works too).
** Digits were chosen because F1..F4 can be captured by the host operating
** system or the desktop (Alt+F4 closes the window, for example).
** Returns TRUE when handled, so the key is not printed as a character.
*/
static bool_t handle_shortcut(uint8_t code)
{
	/* Without a modifier this is an ordinary key */
	if (!g_alt && !g_ctrl)
		return (FALSE);
	if (code < SC_DIGIT1 || code >= SC_DIGIT1 + CONSOLE_COUNT)
		return (FALSE);

	/* The scancodes of 1..4 are consecutive (0x02..0x05), so subtracting the
	** first one yields the console index directly. */
	console_switch((size_t)(code - SC_DIGIT1));
	return (TRUE);
}

/*
** Reads and handles a pending scancode, if any.
** Called continuously from the main loop in kernel_main.
*/
void keyboard_poll(void)
{
	uint8_t status;
	uint8_t scancode;
	uint8_t code;
	bool_t  pressed;
	char    c;

	status = inb(PS2_STATUS_PORT);
	if (!(status & PS2_OUTPUT_FULL))
		return ;
	scancode = inb(PS2_DATA_PORT);

	/* 0xE0 is a prefix: the real code arrives with the next read */
	if (scancode == SC_EXTENDED)
	{
		g_extended = TRUE;
		return ;
	}

	/* Bit 7 tells a release from a press; clearing it gives the key itself.
	** Without this split every key would be handled twice. */
	pressed = (scancode & SC_RELEASE_FLAG) ? FALSE : TRUE;
	code = scancode & (uint8_t)(SC_RELEASE_FLAG - 1);

	if (handle_modifier(code, pressed))
	{
		g_extended = FALSE;
		return ;
	}

	/* Only key presses are handled; releases are ignored */
	if (!pressed || g_extended)
	{
		g_extended = FALSE;
		return ;
	}

	if (handle_shortcut(code))
		return ;

	/* The scancode indexes the table directly; a 0 entry means the key has no
	** printable character (function keys, caps lock, ...). */
	c = g_shift ? g_keymap_shift[code] : g_keymap[code];
	if (c != 0)
		console_putchar(c);
}
