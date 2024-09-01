#include <sys/queue.h>
#include <assert.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <ndk/dpc.h>
#include <ndk/int.h>

void dpc_init(dpc_t *dpc, void (*function)(dpc_t *dpc, void *, void *))
{
	assert(dpc && function);
	dpc->function = function;
	dpc->context1 = nullptr;
	dpc->context2 = nullptr;
	dpc->enqueued = 0;
}

void dpc_enqueue(dpc_t *dpc, void *context1, void *context2)
{
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	irql_t old = spinlock_acquire(&queue->dpc_lock, IRQL_HIGH);
	if (dpc->enqueued) {
		spinlock_release(&queue->dpc_lock, old);
		return;
	}
	dpc->context1 = context1;
	dpc->context2 = context2;
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
	void *context1, *context2;
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	dpc_t *dpc;
	while (1) {
		irql_t old = spinlock_acquire(&queue->dpc_lock, IRQL_HIGH);
		if (TAILQ_EMPTY(&queue->dpcq)) {
			spinlock_release(&queue->dpc_lock, old);
			return;
		}
		dpc = TAILQ_FIRST(&queue->dpcq);
		assert(dpc && dpc->function);
		TAILQ_REMOVE(&queue->dpcq, dpc, queue_entry);
		queue->dpc_count--;
		dpc->enqueued = 0;
		context1 = dpc->context1;
		context2 = dpc->context2;
		spinlock_release(&queue->dpc_lock, old);
		dpc->function(dpc, context1, context2);
	}
}
