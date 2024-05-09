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

static inline cpudata_port_t *get_port_cpudata()
{
	cpudata_port_t *data = 0;
	asm volatile("mov %%gs:0x0, %0" : "=r"(data)::"memory");
	return data;
}
