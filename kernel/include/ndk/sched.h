#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/queue.h>
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/time.h>

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

enum {
	kThreadStateNull,
	kThreadStateReady,
	kThreadStateRunning,
	kThreadStateWaiting,
	kThreadStateDone,
	kThreadStateStandby,
};

typedef struct thread {
	context_t context;
	spinlock_t thread_lock;

	void *kstack_top;

	dpc_t wakeup_dpc;
	timer_t sleep_timer;

	int timeslice; // in ms
	int state;
	int priority;

	TAILQ_ENTRY(thread) entry;
} thread_t;

static_assert(offsetof(struct thread, thread_lock) == sizeof(context_t));

typedef TAILQ_HEAD(, thread) thread_queue_t;

typedef struct scheduler {
	thread_queue_t run_queues[PRIORITY_COUNT];
	unsigned int queue_entries[PRIORITY_COUNT];

	spinlock_t sched_lock;
	dpc_t preemption_dpc;
	timer_t preemption_timer;
} scheduler_t;

void sched_init(scheduler_t *sched);
void sched_kmem_init();
void sched_reschedule();
void sched_resume(thread_t *task);
void sched_preempt();
void sched_yield();
void sched_sleep(uint64_t ns);

[[gnu::noreturn]] void sched_jump_into_idle_thread();

typedef void(thread_start_fn)(void *, void *);

thread_t *sched_alloc_thread();
void sched_destroy_thread(thread_t *task);

void sched_init_thread(thread_t *thread, thread_start_fn startfn, int priority,
		       void *context1, void *context2);

[[gnu::noreturn]] void sched_exit_destroy();

thread_t *curthread();
