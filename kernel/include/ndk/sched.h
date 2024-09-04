#pragma once

#include <stdint.h>
#include <stddef.h>
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
	context_t *context;
	spinlock_t task_lock;

	int priority;

	TAILQ_ENTRY(task) entry;
} task_t;

static_assert(offsetof(struct task, context) == 0);
static_assert(offsetof(struct task, task_lock) == 8);

typedef TAILQ_HEAD(, task) task_queue_t;

typedef struct scheduler {
	task_queue_t task_queues[PRIORITY_COUNT];
	spinlock_t sched_lock;
} scheduler_t;

void sched_init();
void sched_enqueue(task_t *task);
void sched_preempt();

typedef void(task_start_fn)(void *, void *);

task_t *sched_create_task(task_start_fn startfn, int priority, void *context1,
			  void *context2);
void sched_destroy_task(task_t *task);
