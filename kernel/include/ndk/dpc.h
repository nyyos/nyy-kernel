#pragma once

#include "ndk/ndk.h"
#include <sys/queue.h>
#include <stddef.h>

typedef struct dpc {
	TAILQ_ENTRY(dpc) queue_entry;
	void (*function)(struct dpc *dpc, void *context1, void *context2);
	void *context1;
	void *context2;
	int enqueued;
} dpc_t;

typedef struct dpc_queue {
	TAILQ_HEAD(, dpc) dpcq;
	size_t dpc_count;
	spinlock_t dpc_lock;
} dpc_queue_t;

void dpc_init(dpc_t *dpc, void (*function)(dpc_t *dpc, void *, void *));
void dpc_enqueue(dpc_t *dpc, void *context1, void *context2);
void dpc_dequeue(dpc_t *dpc);
void dpc_run_queue();
