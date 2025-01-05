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
	kThreadStateNull = 0,
	kThreadStateReady,
	kThreadStateRunning,
	kThreadStateWaiting,
	kThreadStateDone,
	kThreadStateStandby,
};

typedef struct thread thread_t;
typedef struct obj_header obj_header_t;

enum {
	kWaitblockDequeued = (1 << 0),
	kWaitblockUnwaited = (1 << 1),
};

typedef struct wait_block {
	int flags;
	obj_header_t *object;
	thread_t *thread;
	TAILQ_ENTRY(wait_block) entry;
} wait_block_t;

enum {
	kThreadWaitBlocks = 4,
	kMaxWaitBlocks = 128,
	kWaitTimeoutInfinite = -1,
};

typedef struct thread {
	context_t context;
	spinlock_t thread_lock;

	void *kstack_top;

	timer_t wait_timer;
	dpc_t wait_dpc;
	int wait_count;
	int wait_status;
	wait_block_t *wait_block_array;

	int last_processor;

	int timeslice; // in ms
	int state;
	int priority;

	const char *name;

	TAILQ_ENTRY(thread) entry;
} thread_t;

static_assert(offsetof(struct thread, thread_lock) == sizeof(context_t));

typedef TAILQ_HEAD(, thread) thread_queue_t;

typedef struct scheduler {
	spinlock_t sched_lock;
	thread_queue_t run_queues[PRIORITY_COUNT];
	uint16_t run_mask;

	dpc_t preemption_dpc;
	timer_t preemption_timer;
} scheduler_t;

void sched_init(scheduler_t *sched);
void sched_kmem_init();
void sched_reschedule();
void sched_resume(thread_t *task);
void sched_insert(scheduler_t *sched, thread_t *thread);
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

int sched_wait_single(void *object, long timeout_ms);
int sched_wait_multi(int count, void **objects, long timeout_ms,
		     wait_block_t *wait_block_array);
void sched_unwait(thread_t *thread, int status);
