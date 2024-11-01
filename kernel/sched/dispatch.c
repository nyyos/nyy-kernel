#include "ndk/cpudata.h"
#include "ndk/dpc.h"
#include "ndk/ndk.h"
#include "ndk/time.h"
#include "sys/queue.h"
#include <ndk/sched.h>
#include <ndk/obj.h>
#include <assert.h>

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

	sched_unwait(thread, kWaitStatusTimeout);

cleanup:
	spinlock_release(&thread->thread_lock);
	spinlock_release(&sched->sched_lock);
}

int sched_wait_multi(int count, void **objects, long timeout_ms,
		     wait_block_t *wait_block_array)
{
	thread_t *thread = curthread();
	scheduler_t *sched = &cpudata()->scheduler;

	int max = kMaxWaitBlocks;
	if (wait_block_array == nullptr) {
		wait_block_array = thread->wait_blocks;
		max = kThreadWaitBlocks;
	}

	if (count > max)
		panic("too many objs");

	while (true) {
		irql_t old = spinlock_acquire_raise(&sched->sched_lock);

		bool satisfied = true;
		int satisfieridx = 0;

		for (int i = 0; i < count; i++) {
			wait_block_t *wb = &wait_block_array[i];
			obj_header_t *object = objects[i];

			wb->object = object;
			wb->thread = thread;

			bool signaled = obj_is_signaled(object, thread);

			if (signaled) {
				satisfied = true;
				satisfieridx = i;
				obj_acquire(object, thread);
				break;
			} else {
				satisfied = false;
			}
		}

		if (satisfied) {
			spinlock_release_lower(&sched->sched_lock, old);
			return satisfieridx;
		}

		for (int i = 0; i < count; i++) {
			wait_block_t *wb = &wait_block_array[i];
			obj_header_t *object = objects[i];
			spinlock_acquire(&object->object_lock);
			TAILQ_INSERT_TAIL(&object->waitblock_list, wb, entry);
			object->waitercount++;
			spinlock_release(&object->object_lock);
		}

		thread->wait_block_array = wait_block_array;
		thread->wait_count = count;
		thread->wait_status = kWaitStatusWaiting;
		thread->state = kThreadStateWaiting;

		if (timeout_ms != kWaitTimeoutInfinite) {
			dpc_init(&thread->wait_dpc, wait_timer_exp);
			timer_init(&thread->wait_timer);
			timer_set_in(&thread->wait_timer, MS2NS(timeout_ms),
				     &thread->wait_dpc);
			timer_install(&thread->wait_timer, thread, nullptr);
		}

		spinlock_release_lower(&sched->sched_lock, IRQL_DISPATCH);
		sched_yield();
		irql_lower(old);

		if (thread->wait_status == kWaitStatusTimeout)
			break;
		// XXX: APC
	}

	return thread->wait_status;
}

bool sched_wait_single(void *object, long timeout_ms)
{
	int status = sched_wait_multi(1, &object, timeout_ms, nullptr);
	return status;
}

void sched_unwait(thread_t *thread, int status)
{
	if (thread->wait_status != kWaitStatusWaiting) {
		printk(WARN "unwait on non-waiting thread\n");
		return;
	}

	for (int i = 0; i < thread->wait_count; i++) {
		wait_block_t *wb = &thread->wait_block_array[i];
		obj_header_t *obj = wb->object;
		obj->waitercount--;
		TAILQ_REMOVE(&obj->waitblock_list, wb, entry);
	}

	thread->wait_count = 0;
	thread->wait_block_array = nullptr;

	thread->wait_status = status;

	sched_insert(&cpudata()->scheduler, thread);
}

static void wb_satisfy(wait_block_t *wb)
{
	thread_t *thread = wb->thread;
	int idx = wb - thread->wait_block_array;

	sched_unwait(thread, idx);
}

void obj_satisfy(obj_header_t *hdr, bool all, int status)
{
	wait_block_t *wb;
	while ((wb = TAILQ_FIRST(&hdr->waitblock_list)) != nullptr) {
		thread_t *thread = wb->thread;
		spinlock_acquire(&thread->thread_lock);

		TAILQ_REMOVE(&hdr->waitblock_list, wb, entry);
		hdr->waitercount--;

		spinlock_acquire(&cpudata()->scheduler.sched_lock);
		wb_satisfy(wb);
		spinlock_release(&cpudata()->scheduler.sched_lock);

		spinlock_release(&thread->thread_lock);

		if (!all)
			return;
	}
}
