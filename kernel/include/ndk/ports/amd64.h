#pragma once

#include <stdint.h>

#include <ndk/addr.h>

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

typedef struct vm_port_map {
	paddr_t cr3;
} vm_port_map_t;
