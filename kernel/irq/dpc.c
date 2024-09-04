#include <sys/queue.h>
#include <assert.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <ndk/dpc.h>
#include <ndk/int.h>

static int acquire_dpc_lock(cpudata_t *data)
{
	int oldstate = port_set_ints(0);
	spinlock_acquire(&data->dpc_queue.dpc_lock);
	return oldstate;
}

static void release_dpc_lock(cpudata_t *data, int old)
{
	spinlock_release(&data->dpc_queue.dpc_lock);
	port_set_ints(old);
}

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
	int oldstate = acquire_dpc_lock(cpudata());
	if (dpc->enqueued) {
		release_dpc_lock(cpudata(), oldstate);
		return;
	}
	dpc->context1 = context1;
	dpc->context2 = context2;
	queue->dpc_count++;
	TAILQ_INSERT_TAIL(&queue->dpcq, dpc, queue_entry);
	dpc->enqueued = 1;
	softint_issue(IRQL_DISPATCH);
	release_dpc_lock(cpudata(), oldstate);
}

void dpc_dequeue(dpc_t *dpc)
{
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	int oldstate = acquire_dpc_lock(cpudata());
	TAILQ_REMOVE(&queue->dpcq, dpc, queue_entry);
	release_dpc_lock(cpudata(), oldstate);
}

/* IRQL is at DPC level at entry */
void dpc_run_queue()
{
	void *context1, *context2;
	dpc_queue_t *queue = &cpudata()->dpc_queue;
	dpc_t *dpc;
	while (1) {
		int oldstate = acquire_dpc_lock(cpudata());
		if (TAILQ_EMPTY(&queue->dpcq)) {
			release_dpc_lock(cpudata(), oldstate);
			return;
		}
		dpc = TAILQ_FIRST(&queue->dpcq);
		assert(dpc && dpc->function);
		TAILQ_REMOVE(&queue->dpcq, dpc, queue_entry);
		queue->dpc_count--;
		dpc->enqueued = 0;
		context1 = dpc->context1;
		context2 = dpc->context2;
		release_dpc_lock(cpudata(), oldstate);
		dpc->function(dpc, context1, context2);
	}
}
