#include "ndk/cpudata.h"
#include "ndk/dpc.h"
#include "ndk/ndk.h"
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

	extern thread_t *fwt;
	while (true) {
		if (curthread() == fwt) {
			printk("start fireworks wait\n");
		}
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
			} else if (!signaled) {
				satisfied = false;
			}
		}

		if (satisfied) {
			spinlock_release_lower(&sched->sched_lock, old);
			return STATUS_RANGE_WAIT + satisfieridx;
		} else if (timeout_ms == 0) {
			spinlock_release_lower(&sched->sched_lock, old);
			return STATUS_TIMEOUT;
		}

		printk("start inserting shit\n");
		for (int i = 0; i < count; i++) {
			wait_block_t *wb = &wait_block_array[i];
			obj_header_t *object = objects[i];
			spinlock_acquire(&object->wb_lock);
			object->waitercount++;
			TAILQ_INSERT_TAIL(&object->waitblock_list, wb, entry);
			printk("insert %p into %p\n", wb,
			       &object->waitblock_list);
			spinlock_release(&object->wb_lock);
		}
		printk("finish inserting shit\n");

		thread->wait_block_array = wait_block_array;
		thread->wait_count = count;
		thread->wait_status = STATUS_WAITING;
		thread->state = kThreadStateWaiting;

		if (timeout_ms != kWaitTimeoutInfinite) {
			printk(WARN "TIMEOUT TIMER %lx\n", timeout_ms);
			dpc_init(&thread->wait_dpc, wait_timer_exp);
			timer_init(&thread->wait_timer);
			timer_set_in(&thread->wait_timer, MS2NS(timeout_ms),
				     &thread->wait_dpc);
			timer_install(&thread->wait_timer, thread, nullptr);
		}

		spinlock_release_lower(&sched->sched_lock, IRQL_DISPATCH);
		sched_yield();
		irql_lower(old);

		if (thread->wait_status == STATUS_TIMEOUT)
			break;

		printk("thread status = %d\n", thread->wait_status);
		if (thread->wait_status == STATUS_WAITING) {
			printk(ERR "fuck this Bug\n");
			//sched_yield();
		}
		assert(thread->wait_status != STATUS_WAITING);

		// XXX: APC
	}

	if (curthread() == fwt) {
		printk("exit fireworks wait\n");
	}

	return thread->wait_status;
}

int sched_wait_single(void *object, long timeout_ms)
{
	int status = sched_wait_multi(1, &object, timeout_ms, nullptr);
	if (status == STATUS_WAIT(0))
		return STATUS_OK;
	return status;
}

void sched_unwait(thread_t *thread, int status)
{
	if (thread->state == kThreadStateReady ||
	    thread->state == kThreadStateStandby) {
		printk(ERR "%s %d %d\n", thread->name, thread->wait_status,
		       thread->state);
	}
	assert(thread->state != kThreadStateReady ||
	       thread->state != kThreadStateStandby ||
	       thread->state != kThreadStateRunning);
	if (thread->wait_status != STATUS_WAITING) {
		printk(WARN "unwait on non-waiting thread (%s)\n",
		       thread->name);
		return;
	}

	for (int i = 0; i < thread->wait_count; i++) {
		wait_block_t *wb = &thread->wait_block_array[i];
		obj_header_t *obj = wb->object;
		spinlock_acquire(&obj->wb_lock);
		printk("remove %p from %p\n", wb, obj);
		TAILQ_REMOVE(&obj->waitblock_list, wb, entry);
		memset(wb, 0x0, sizeof(*wb));
		obj->waitercount--;
		assert(obj->waitercount >= 0 && "fuck");
		if (obj->waitercount > 0) {
			printk("obj:wc=%d t=%s\n", obj->waitercount,
			       thread->name);
		}
		spinlock_release(&obj->wb_lock);
	}

	thread->wait_status = status;
	thread->wait_count = -1;
	thread->wait_block_array = nullptr;

	sched_insert(&cpudata()->scheduler, thread);
}

void obj_satisfy(obj_header_t *hdr, bool all, int status)
{
	irql_t ipl = spinlock_acquire_raise(&cpudata()->scheduler.sched_lock);
	wait_block_t *wb;
	while ((wb = TAILQ_FIRST(&hdr->waitblock_list)) != nullptr) {
		thread_t *thread = wb->thread;
		spinlock_acquire(&thread->thread_lock);
		assert(thread->wait_count != -1);

		int idx = wb - thread->wait_block_array;
		printk("start:%p blk:%p idx:%d\n", thread->wait_block_array, wb,
		       idx);

		sched_unwait(thread, STATUS_RANGE_WAIT + idx);

		spinlock_release(&thread->thread_lock);

		if (!all)
			break;
	}

	spinlock_release_lower(&cpudata()->scheduler.sched_lock, ipl);
}
