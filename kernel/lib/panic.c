#include <ndk/ndk.h>
#include <ndk/port.h>
#include "ndk/vm.h"
#include <ndk/internal/symbol.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

static const char *panic_art =
	"\n"
	" __  ___  _______ .______      .__   __.  _______  __      \n"
	"|  |/  / |   ____||   _  \\     |  \\ |  | |   ____||  |     \n"
	"|  '  /  |  |__   |  |_)  |    |   \\|  | |  |__   |  |     \n"
	"|    <   |   __|  |      /     |  . `  | |   __|  |  |     \n"
	"|  .  \\  |  |____ |  |\\  \\----.|  |\\   | |  |____ |  `----.\n"
	"|__|\\__\\ |_______|| _| `._____||__| \\__| |_______||_______|\n"
	"                                                           \n"
	".______      ___      .__   __.  __    ______              \n"
	"|   _  \\    /   \\     |  \\ |  | |  |  /      |             \n"
	"|  |_)  |  /  ^  \\    |   \\|  | |  | |  ,----'             \n"
	"|   ___/  /  /_\\  \\   |  . `  | |  | |  |                  \n"
	"|  |     /  _____  \\  |  |\\   | |  | |  `----.             \n"
	"| _|    /__/     \\__\\ |__| \\__| |__|  \\______|             \n"
	"                                                           \n"
	"";

[[gnu::noreturn]] void panic(const char *msg)
{
	_printk_consoles_write(panic_art, strlen(panic_art));
	printk(PANIC "%s\n", msg);

	vmstat_dump();

	printk("\nStacktrace:\n");

	uint64_t *rbp, *rip;
	asm volatile("mov %%rbp, %0" : "=r"(rbp));
	while (rbp) {
		rip = rbp + 1;
		if (!rip || *rip == 0)
			break;

		symbol_t *sym = symbols_lookup_address(*rip);
		if (sym) {
			printk(" 0x%lx - %s+0x%lx\n", *rip, sym->symname,
			       *rip - sym->base - symbols_offset());
		} else {
			printk(" 0x%lx\n", *rip);
		}

		rbp = (uint64_t *)*rbp;
	}

	printk("\n");

	hcf();
	__builtin_trap();
}
