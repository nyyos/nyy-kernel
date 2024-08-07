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

void port_init_bsp(cpudata_t *bsp_data);
void port_data_common_init(cpudata_t *data);
