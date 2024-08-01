#pragma once

#include <sys/queue.h>
#include <stddef.h>

typedef unsigned int irql_t;

enum IRQL_LEVELS {
	PASSIVE_LEVEL = 0,
	APC_LEVEL = 1,
	DPC_LEVEL = 2,
	CLOCK_LEVEL = 14,
	HIGH_LEVEL = 15,
};

typedef struct dpc {
	TAILQ_ENTRY(dpc) queue_entry;
	void (*function)(void *data);
	void *data;
} dpc_t;

typedef struct dpc_queue {
	TAILQ_HEAD(, dpc) queue;
	size_t dpc_count;
} dpc_queue_t;

/* DPC is not allowed to be on any queue */
void dpc_queue_insert(dpc_t *dpc);
/* if dpc is NOT on the queue, NOP */
void dpc_queue_remove(dpc_t *dpc);
/* raises IRQL to DISPATCH_LEVEL */
void dpc_queue_execute();

irql_t irql_raise(irql_t level);
void irql_lower(irql_t level);
/* provided by port */
irql_t irql_current();
void irql_set(irql_t irql);
