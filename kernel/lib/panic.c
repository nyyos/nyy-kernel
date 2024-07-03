#include <ndk/ndk.h>
#include <ndk/port.h>
#include <stdarg.h>

const char *panic_art =
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
	pac_printf("%s\n%s\nStacktrace:\n", panic_art, msg);

	uint64_t *rbp, *rip;
	asm volatile("mov %%rbp, %0" : "=r"(rbp));
	while (rbp) {
		rip = rbp + 1;
		if (*rip == 0)
			break;
		pac_printf(" 0x%lx - %s\n", *rip, "TODO: SYMBOL LOOKUP");
		rbp = (uint64_t *)*rbp;
	}

	pac_printf("\n");

	hcf();
}
