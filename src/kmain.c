#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];

/* Maximum line buffer size for the shell */
#define SHELL_LINE_MAX 256

/* Print the shell prompt */
static void shell_prompt(void)
{
	serial_puts("> ");
}

/* Parse and run the command in `line` */
static void shell_exec(char *line)
{
	serial_puts("\n");

	if (strcmp(line, "uptime") == 0) {
		/* Print seconds since boot as "<N>s" */
		u64 ticks = timer_read();
		u64 secs  = ticks / TIMER_FREQ;
		char buf[32];
		snprintf(buf, sizeof(buf), "%llus\n", secs);
		serial_puts(buf);

	} else if (strncmp(line, "echo ", 5) == 0) {
		/* Print everything after "echo " */
		serial_puts(line + 5);
		serial_puts("\n");

	} else if (strncmp(line, "alarm ", 6) == 0) {
		/* Parse the seconds argument and schedule an alarm */
		u64 secs = strtou64(line + 6, 10);
		timer_set_alarm(secs);

	} else if (strcmp(line, "echo") == 0) {
		/* bare echo with no argument — print empty line */
		serial_puts("\n");
	}
	/* Unknown commands are silently ignored */
}

void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	/* Line accumulation buffer */
	char line[SHELL_LINE_MAX];
	size_t line_len = 0;

	/* Temporary read buffer */
	char rbuf[SHELL_LINE_MAX];

	shell_prompt();

	while (1) {
		/* Read whatever bytes the interrupt handler has buffered */
		size_t n = serial_read(rbuf);

		for (size_t i = 0; i < n; i++) {
			char c = rbuf[i];

			if (c == '\r' || c == '\n') {
				/* Carriage return: execute the accumulated command */
				line[line_len] = '\0';
				shell_exec(line);
				line_len = 0;
				shell_prompt();

			} else if (c == '\b' || c == 127) {
				/* Backspace / DEL: erase last character */
				if (line_len > 0) {
					line_len--;
					/* Erase on terminal: BS + space + BS */
					serial_puts("\b \b");
				}

			} else if (line_len < SHELL_LINE_MAX - 1) {
				/* Normal printable character: accumulate and echo */
				line[line_len++] = c;
				serial_putc(c);
			}
		}
	}
}
