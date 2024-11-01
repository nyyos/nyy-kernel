#include "ndk/obj.h"
#include <ndk/ndk.h>
#include <ndk/mutex.h>
#include <ndk/sched.h>
#include <assert.h>

void mutex_init(mutex_t *mutex)
{
	obj_init(&mutex->hdr, kObjTypeMutex);
	mutex->owner = nullptr;
}

void mutex_release(mutex_t *mutex)
{
	assert(mutex->owner == curthread());
	if (--mutex->hdr.signalcount == 0) {
		mutex->owner = nullptr;
		spinlock_release(&mutex->hdr.object_lock);
		obj_satisfy(&mutex->hdr, false, 0);
	}
}
