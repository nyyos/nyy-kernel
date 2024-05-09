#pragma once

#include "tss.h"

void cpu_gdt_init();
void cpu_gdt_load();
void cpu_tss_load(tss_t *tss);
