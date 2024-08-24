#pragma once

#include "ndk/ndk.h"
#include <sys/queue.h>
#include <stddef.h>

typedef struct dpc {
	TAILQ_ENTRY(dpc) queue_entry;
	void (*function)(void *private);
	void *private;
	int enqueued;
} dpc_t;

typedef struct dpc_queue {
	TAILQ_HEAD(, dpc) dpcq;
	size_t dpc_count;
	spinlock_t dpc_lock;
} dpc_queue_t;

void dpc_init(dpc_t *dpc, void (*function)(void *), void *private);
void dpc_enqueue(dpc_t *dpc);
void dpc_dequeue(dpc_t *dpc);
void dpc_run_queue();
