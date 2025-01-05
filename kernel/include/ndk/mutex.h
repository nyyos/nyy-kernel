#pragma once

#include "ndk/event.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <ndk/sched.h>
#include <ndk/obj.h>

typedef struct mutex {
	obj_header_t tmp_hdr;
	event_t event;

	thread_t *owner;
} mutex_t;

void mutex_init(mutex_t *mutex);
int mutex_acquire(mutex_t *mutex, long timeout);
void mutex_release(mutex_t *mutex);

#ifdef __cplusplus
}
#endif
