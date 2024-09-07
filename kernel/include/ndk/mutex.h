#pragma once

#include <stdint.h>
#include <ndk/sched.h>

typedef struct mutex {
	thread_t *owner;
	volatile uint8_t flag;

	spinlock_t queue_lock;
	thread_queue_t wait_queue;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_acquire(mutex_t *mutex);
void mutex_release(mutex_t *mutex);
