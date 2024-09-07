#include <dkit/console.h>

volatile unsigned char *uart = (unsigned char *)0x10000000;

static void serial_putc(int c)
{
	*uart = c;
}

void serial_write(console_t *, const char *msg, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		serial_putc(msg[i]);
	}
}

static console_t serial_console = {
	.name = "serial",
	.write = serial_write,
};

void limine_bootconsoles_init()
{
	console_add(&serial_console);
}

void _start(void)
{
}
