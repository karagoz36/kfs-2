/*
** shell.c — a minimal debugging shell (bonus).
**
** Not a POSIX shell: characters are collected into a line, and when Enter is
** pressed the line is compared with a small table of commands.
*/

#include "shell.h"
#include "console.h"
#include "printk.h"
#include "string.h"
#include "keyboard.h"
#include "gdt.h"
#include "stack.h"

#define LINE_MAX 64
#define PROMPT   "kfs> "

typedef struct s_command
{
	const char *name;
	const char *help;
	void        (*run)(void);
}	t_command;

static char   g_line[LINE_MAX];
static size_t g_len = 0;

static void cmd_help(void);

/* Halt: make sure interrupts stay off, then 'hlt' stops the CPU for good. */
static void cmd_halt(void)
{
	printk("System halted.\n");
	__asm__ volatile("cli");
	while (TRUE)
		__asm__ volatile("hlt");
}

static const t_command g_commands[] = {
	{"help",   "list the commands",       cmd_help},
	{"stack",  "print the kernel stack",  print_kernel_stack},
	{"gdt",    "print the GDT",           gdt_print},
	{"clear",  "clear the screen",        console_clear},
	{"reboot", "restart the machine",     keyboard_reboot},
	{"halt",   "stop the CPU",            cmd_halt},
};
#define COMMAND_COUNT (sizeof(g_commands) / sizeof(g_commands[0]))

static void cmd_help(void)
{
	size_t i = 0;
	size_t pad;

	while (i < COMMAND_COUNT)
	{
		printk("  %s", g_commands[i].name);
		pad = k_strlen(g_commands[i].name);
		while (pad++ < 8)
			console_putchar(' ');
		printk("%s\n", g_commands[i].help);
		i++;
	}
}

/* Runs the typed line. Spaces before and after the command are ignored. */
static void run_line(void)
{
	const char *cmd = g_line;
	size_t      i = 0;

	while (g_len > 0 && g_line[g_len - 1] == ' ')
		g_len--;
	g_line[g_len] = '\0';
	while (*cmd == ' ')
		cmd++;
	if (*cmd == '\0')
		return ;
	while (i < COMMAND_COUNT)
	{
		if (k_strcmp(cmd, g_commands[i].name) == 0)
		{
			g_commands[i].run();
			return ;
		}
		i++;
	}
	printk("unknown command: '%s' (try 'help')\n", cmd);
}

void shell_init(void)
{
	printk(PROMPT);
}

/*
** Called for every character typed. Enter runs the line, backspace erases
** the last character, anything printable is echoed and stored.
*/
void shell_input(char c)
{
	if (c == '\n')
	{
		console_putchar('\n');
		run_line();
		g_len = 0;
		printk(PROMPT);
	}
	else if (c == '\b')
	{
		if (g_len > 0)
		{
			g_len--;
			console_putchar('\b');
		}
	}
	else if (c >= ' ' && g_len < LINE_MAX - 1)
	{
		g_line[g_len++] = c;
		console_putchar(c);
	}
}
