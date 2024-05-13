#pragma once

#define PAGE_SIZE 0x1000

typedef struct cpudata_port {
} cpudata_port_t;

static inline void hcf()
{
	for (;;) {
		asm volatile("wfi");
	}
}
