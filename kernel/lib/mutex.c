#include "ndk/event.h"
#include "ndk/obj.h"
#include "ndk/status.h"
#include <ndk/ndk.h>
#include <ndk/mutex.h>
#include <ndk/sched.h>
#include <assert.h>

void mutex_init(mutex_t *mutex)
{
	obj_init(&mutex->tmp_hdr, 123);
	event_init(&mutex->event, false, true);
	mutex->owner = nullptr;
}

int mutex_acquire(mutex_t *mutex, long timeout)
{
	while (true) {
		irql_t irql = OBJ_ACQUIRE(&mutex->event);
		if (mutex->event.hdr.signalcount == 1) {
			assert(mutex->owner == nullptr);
			mutex->owner = curthread();
			mutex->event.hdr.signalcount = 0;
			OBJ_RELEASE(&mutex->event, irql);
			return STATUS_OK;
		}
		OBJ_RELEASE(&mutex->event, irql);
		if (sched_wait_single(&mutex->event, timeout)) {
			return STATUS_TIMEOUT;
		}
	}
}

void mutex_release(mutex_t *mutex)
{
	assert(mutex->owner == curthread());
	irql_t irql = OBJ_ACQUIRE(&mutex->event);
	mutex->owner = nullptr;
	OBJ_RELEASE(&mutex->event, irql);
	event_signal(&mutex->event);
}
