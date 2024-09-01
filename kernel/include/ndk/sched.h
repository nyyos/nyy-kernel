#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <ndk/ndk.h>
#include <ndk/port.h>

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
	cpu_state_t state;
	int priority;

	TAILQ_ENTRY(task) entry;
} task_t;

typedef TAILQ_HEAD(, task) task_queue_t;

typedef struct scheduler {
	task_queue_t task_queues[PRIORITY_COUNT];
	spinlock_t sched_lock;
} scheduler_t;

void sched_init();
void sched_enqueue(task_t *task);
task_t *sched_create_task(void *ip, int priority);
void sched_destroy_task(task_t *task);
