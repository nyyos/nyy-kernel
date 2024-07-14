#include <ndk/port.h>
#include <string.h>
#include "gdt.h"

void port_cpu_state_init(cpu_state_t *stp, bool kern)
{
	memset(stp, 0x0, sizeof(cpu_state_t));
	stp->rflags = 0x200;

	if (kern) {
		stp->cs = kGdtKernelCode * 8;
		stp->ss = kGdtKernelData * 8;
	} else {
		stp->cs = kGdtUserCode * 8;
		stp->ss = kGdtUserData * 8;
	}
}
