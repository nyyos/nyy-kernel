#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ndk/sched.h>
#include <ndk/obj.h>

typedef struct mutex {
	obj_header_t hdr;

	thread_t *owner;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_release(mutex_t *mutex);

#ifdef __cplusplus
}
#endif
