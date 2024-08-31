#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <ndk/ndk.h>

enum {
	kPriorityIdle = 0,
	kPriorityLow = 1,
	kPriorityLowMax = 4,
	kPriorityMiddle = 5,
	kPriorityMiddleMax = 9,
	kPriorityHigh = 10,
	kPriorityHighMax = 15,
	PRIORITY_COUNT = 16
};

typedef struct task {
} task_t;

typedef TAILQ_HEAD(, task) task_queue_t;

typedef struct scheduler {
	uint16_t schedule_mask; // if a bit is set, a task is available
	task_queue_t task_queues[PRIORITY_COUNT];
	spinlock_t sched_lock;
} scheduler_t;
