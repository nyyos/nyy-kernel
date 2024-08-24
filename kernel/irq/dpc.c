#include "ndk/irql.h"
#include <sys/queue.h>
#include <assert.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <ndk/dpc.h>
#include <ndk/int.h>

void dpc_init(dpc_t *dpc, void (*function)(void *), void *private)
{
	assert(dpc && function);
	dpc->function = function;
	dpc->private = private;
	dpc->enqueued = 0;
}

void dpc_enqueue(dpc_t *dpc)
{
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	irql_t old = spinlock_acquire(&queue->dpc_lock, IRQL_HIGH);
	if (dpc->enqueued) {
		spinlock_release(&queue->dpc_lock, old);
		return;
	}
	queue->dpc_count++;
	TAILQ_INSERT_TAIL(&queue->dpcq, dpc, queue_entry);
	dpc->enqueued = 1;
	softint_issue(IRQL_DISPATCH);
	spinlock_release(&queue->dpc_lock, old);
}

void dpc_dequeue(dpc_t *dpc)
{
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	irql_t old = spinlock_acquire(&queue->dpc_lock, IRQL_HIGH);
	TAILQ_REMOVE(&queue->dpcq, dpc, queue_entry);
	spinlock_release(&queue->dpc_lock, old);
}

/* raises IRQL to DISPATCH_LEVEL */
void dpc_run_queue()
{
	irql_t old = irql_raise(IRQL_DISPATCH);
	softint_ack(IRQL_DISPATCH);
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	dpc_t *dpc;
	while (!TAILQ_EMPTY(&queue->dpcq)) {
		dpc = TAILQ_FIRST(&queue->dpcq);
		assert(dpc && dpc->function);
		TAILQ_REMOVE(&queue->dpcq, dpc, queue_entry);
		queue->dpc_count--;
		dpc->function(dpc->private);
	}
	irql_lower(old);
}
