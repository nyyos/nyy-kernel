#pragma once

#include <stddef.h>
#include <stdlib.h>

extern size_t PAGE_SIZE;
extern size_t MEM_KERNEL_START;
extern size_t MEM_KERNEL_BASE;

#define MEM_KERNEL_SIZE MiB(256)

typedef struct cpudata_port {
} cpudata_port_t;

static inline void hcf()
{
	abort();
}

typedef struct vm_port_map {
} vm_port_map_t;

typedef struct [[gnu::packed]] cpu_state {
} interrupt_frame_t;

static inline void port_enable_ints()
{
}

static inline void port_disable_ints()
{
}

static inline void port_wait_nextint()
{
}

#define IRQ_VECTOR_MAX (128)
#define IRQL_VECTOR_COUNT_PER 8
#define IRQL_COUNT 16

enum IRQL_LEVELS {
	IRQL_PASSIVE = 0,
	IRQL_APC = 1,
	IRQL_DISPATCH = 2,
	IRQL_DEVICE = 3,
	IRQL_CLOCK = 13,
	IRQL_IPI = 14,
	IRQL_HIGH = 15,
};

#define IRQL_TO_VECTOR(irql) (((irql) << 2))
#define VECTOR_TO_IRQL(vec) ((vec) >> 2)
