#pragma once

#define PAGE_SIZE 0x1000

#define KSTACK_SIZE PAGE_SIZE * 2

typedef struct cpudata_port {
} cpudata_port_t;

static inline void hcf()
{
	for (;;) {
		asm volatile("wfi");
	}
}

typedef struct vm_port_map {
} vm_port_map_t;

typedef struct [[gnu::packed]] context {
} context_t;


