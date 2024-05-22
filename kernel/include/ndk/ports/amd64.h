#pragma once

#include <ndk/addr.h>
#include <stdint.h>

#define PAGE_SIZE 0x1000

#define MEM_KERNEL_START 0xFFFFE00000000000L
#define MEM_KERNEL_SIZE TiB(1)

typedef struct cpudata_port {
	struct cpudata_port *self;

	uint64_t lapic_id;
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
