#include "ndk/cpudata.h"
#include "ndk/dpc.h"
#include "ndk/irql.h"
#include "ndk/mutex.h"
#include "ndk/ndk.h"
#include "ndk/ports/amd64.h"
#include "ndk/status.h"
#include "ndk/time.h"
#include "sys/queue.h"
#include <ndk/sched.h>
#include <ndk/obj.h>
#include <assert.h>
#include <string.h>

static void wait_timer_exp(dpc_t *, void *ctx1, void *)
{
	scheduler_t *sched = &cpudata()->scheduler;
	spinlock_acquire(&sched->sched_lock);
	thread_t *thread = ctx1;
	spinlock_acquire(&thread->thread_lock);
	assert(thread);
	if (thread->state != kThreadStateWaiting) {
		goto cleanup;
	}

	printk("timeout!\n");
	sched_unwait(thread, STATUS_TIMEOUT);

cleanup:
	spinlock_release(&thread->thread_lock);
	spinlock_release(&sched->sched_lock);
}

static void dequeue_wb(wait_block_t *wb)
{
	obj_header_t *obj = wb->object;
	OBJ_ACQUIRE_ELEVATED(obj);
	if ((wb->flags & kWaitblockDequeued) == 0) {
		TAILQ_REMOVE(&obj->waitblock_list, wb, entry);
		if (obj->waitercount == 0) {
			panic("underflow");
		}
		obj->waitercount--;
	}
	OBJ_RELEASE_ELEVATED(obj);
}

int wait_thread(thread_t *thread)
{
	thread->state = kThreadStateWaiting;
	sched_yield();

	wait_block_t *waitblock = thread->wait_block_array;
	size_t count = thread->wait_count;

	while (count--) {
		dequeue_wb(waitblock);
		waitblock++;
	}

	return thread->wait_status;
}

int sched_wait_multi(int count, void **objects, long timeout_ms,
		     wait_block_t *wait_block_array)
{
	assert(!"todo");
}

int sched_wait_single(void *object, long timeout_ms)
{
	thread_t *thread = curthread();
	obj_header_t *hdr = object;
	assert(hdr->type != 123);
	irql_t irql = irql_raise(IRQL_DISPATCH);

	wait_block_t singlewb;
	singlewb.object = hdr;
	singlewb.thread = thread;
	singlewb.flags = 0;

	thread->wait_status = STATUS_WAITING;
	thread->wait_block_array = &singlewb;
	thread->wait_count = 1;

	OBJ_ACQUIRE_ELEVATED(&hdr->object_lock);

	if (hdr->signalcount) {
		// object already signaled

		if (hdr->type != kObjTypeEventNotif) {
			hdr->signalcount -= 1;
		}

		OBJ_RELEASE_ELEVATED(&hdr->object_lock);
		thread->wait_status = STATUS_OK;

		irql_lower(irql);

		return STATUS_OK;
	}

	TAILQ_INSERT_TAIL(&hdr->waitblock_list, &singlewb, entry);
	++hdr->waitercount;

	OBJ_RELEASE_ELEVATED(&hdr->object_lock);

	// XXX: timeout

	spinlock_acquire(&thread->thread_lock);
	int status = wait_thread(thread);
	irql_lower(irql);

	return status;
}

void sched_unwait(thread_t *thread, int status)
{
	if (thread->state == kThreadStateReady ||
	    thread->state == kThreadStateStandby) {
		printk(ERR "%s %d %d\n", thread->name, thread->wait_status,
		       thread->state);
	}
	assert(thread->state == kThreadStateWaiting);
	if (thread->wait_status != STATUS_WAITING) {
		printk(WARN "unwait on non-waiting thread (%s)\n",
		       thread->name);
		return;
	}

	thread->wait_status = status;

	wait_block_t *wb = thread->wait_block_array;
	size_t count = thread->wait_count;

	printk("count: %ld\n", count);
	printk("wb: %p\n", wb);
	printk("name: %s\n", thread->name);

	while (count--) {
		wb->flags |= kWaitblockUnwaited;
		wb++;
	}

	sched_resume(thread);
}

void obj_satisfy(obj_header_t *hdr, bool all, int status)
{
	printk("satisfy object\n");
	assert(spinlock_held(&hdr->object_lock));
	wait_block_t *wb;
	while ((wb = TAILQ_FIRST(&hdr->waitblock_list)) != nullptr) {
		thread_t *thread = wb->thread;
		spinlock_acquire(&thread->thread_lock);
		assert(thread->wait_count != -1);
		wb->flags |= kWaitblockDequeued;
		TAILQ_REMOVE(&hdr->waitblock_list, wb, entry);
		hdr->waitercount--;
		if (wb->flags & kWaitblockUnwaited) {
			spinlock_release(&thread->thread_lock);
			continue;
		}

		int idx = wb - thread->wait_block_array;
		sched_unwait(thread, STATUS_RANGE_WAIT + idx);
		spinlock_release(&thread->thread_lock);
		if (!all)
			break;
	}
}
