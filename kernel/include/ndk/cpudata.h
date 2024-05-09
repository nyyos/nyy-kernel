#pragma once

#include <ndk/port.h>
#include <ndk/util.h>

typedef struct cpudata {
	cpudata_port_t port_data;
	bool bsp;
} cpudata_t;

static inline cpudata_t *cpudata()
{
	return (cpudata_t *)get_port_cpudata();
}

void cpudata_setup(cpudata_t *cpudata);
void cpudata_port_setup(cpudata_port_t *cpudata);
