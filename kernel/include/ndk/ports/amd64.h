#pragma once

#include <stdint.h>

#define PAGE_SIZE 0x1000

typedef struct cpudata_port {
	struct cpudata_port *self;
} cpudata_port_t;

static inline void hcf()
{
	for (;;) {
		asm volatile("cli;hlt");
	}
}
