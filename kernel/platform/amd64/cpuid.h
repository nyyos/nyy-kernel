#pragma once

#include "asm.h"

static inline void cpuid_vendor(char vendor[13])
{
	uint32_t unused, b, c, d;
	cpuid(0, &unused, &b, &c, &d);
	vendor[0] = b & 0xFF;
	vendor[1] = (b >> 8) & 0xFF;
	vendor[2] = (b >> 16) & 0xFF;
	vendor[3] = (b >> 24) & 0xFF;
	vendor[4] = d & 0xFF;
	vendor[5] = (d >> 8) & 0xFF;
	vendor[6] = (d >> 16) & 0xFF;
	vendor[7] = (d >> 24) & 0xFF;
	vendor[8] = c & 0xFF;
	vendor[9] = (c >> 8) & 0xFF;
	vendor[10] = (c >> 16) & 0xFF;
	vendor[11] = (c >> 24) & 0xFF;
	vendor[12] = '\0';
}

typedef struct cpu_features {
	// 1gb pages
	int gbpages;
	// pte no execute bit
	int nx;
	// global pages
	int pge;
	int pat;
	int pcid;
} cpu_features_t;

extern cpu_features_t g_features;
