#pragma once

#include "tss.h"

enum gdt_entries {
	kGdtNull = 0,
	kGdtKernelCode,
	kGdtKernelData,
	kGdtUserData,
	kGdtUserCode,
	kGdtTss,
};

void cpu_gdt_init();
void cpu_gdt_load();
void cpu_tss_load(tss_t *tss);
