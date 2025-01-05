#include "ndk/obj.h"
#include <ndk/event.h>

int event_reset(event_t *event)
{
	printk("reset\n");
	irql_t irql = spinlock_acquire_raise(&event->hdr.object_lock);
	int old = event->hdr.signalcount;
	event->hdr.signalcount = 0;
	spinlock_release_lower(&event->hdr.object_lock, irql);
	return old;
}

void event_init(event_t *event, bool notif, bool signaled)
{
	if (notif) {
		obj_init(&event->hdr, kObjTypeEventNotif);
	} else {
		obj_init(&event->hdr, kObjTypeEvent);
	}

	event->hdr.signalcount = signaled ? 1 : 0;
}

int event_signal(event_t *event)
{
	irql_t irql = OBJ_ACQUIRE(event);

	if (event->hdr.signalcount) {
		OBJ_RELEASE(event, irql);
		return 1;
	}

	if (event->hdr.waitercount) {
		if (event->hdr.type == kObjTypeEventNotif) {
			// satisfy All
			obj_satisfy(&event->hdr, true, 0);

			event->hdr.signalcount = 1;
		} else {
			obj_satisfy(&event->hdr, false, 0);
		}
	} else {
		event->hdr.signalcount = 1;
	}

	OBJ_RELEASE(event, irql);
	return 0;
}
