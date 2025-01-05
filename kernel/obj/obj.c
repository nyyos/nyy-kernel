#include "ndk/obj.h"
#include "ndk/mutex.h"
#include "ndk/ndk.h"
#include <assert.h>

void obj_init(void *hdr, int type)
{
	obj_header_t *obj = hdr;
	SPINLOCK_INIT(&obj->object_lock);
	obj->type = type;
	obj->refcnt = 1;
	obj->ops = nullptr;
	obj->signalcount = 0;
	obj->waitercount = 0;
	TAILQ_INIT(&obj->waitblock_list);
}

void obj_retain(void *hdr)
{
	__atomic_fetch_add(&((obj_header_t *)hdr)->refcnt, 1, __ATOMIC_ACQUIRE);
}

void obj_release(void *hdr)
{
	obj_header_t *obj = hdr;
	if (0 == __atomic_sub_fetch(&obj->refcnt, 1, __ATOMIC_RELEASE)) {
		obj->ops->free(hdr);
	}
}

bool obj_is_signaled(obj_header_t *hdr, thread_t *thread)
{
	assert(hdr);
	if (hdr->type == kObjTypeMutex) {
		if (hdr->signalcount == 0)
			return true;

		mutex_t *mtx = (mutex_t *)hdr;
		assert(mtx->owner != thread);
		return false;
	}
	return hdr->signalcount != 0;
}
