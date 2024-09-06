#pragma once

#include <limine.h>
#include <ndk/cpudata.h>
#include <ndk/ndk.h>

#ifdef CONFIG_SMP
struct smp_info {
	bool ready;
	cpudata_t data;
};
#endif

void early_port_bsp_init(cpudata_t *bsp_data);
void early_port_cpu_common_init(cpudata_t *data);
