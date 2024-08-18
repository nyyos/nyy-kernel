#include "ndk/ports/amd64.h"
#include <ultra_protocol.h>
#include "ndk/ndk.h"

extern void _port_init_boot_consoles();

void ultra_entry(struct ultra_boot_context *ctx, uint32_t magic)
{
	if (magic != ULTRA_MAGIC)
		__builtin_trap();
	_port_init_boot_consoles();
	printk(DEBUG "TEST\n");
	hcf();
}
