#pragma once

#include <stddef.h>
#include <stdlib.h>

extern size_t PAGE_SIZE;

typedef struct cpudata_port {
} cpudata_port_t;

static inline void hcf()
{
	abort();
}
