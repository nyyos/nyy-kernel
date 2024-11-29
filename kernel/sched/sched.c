#include <sys/queue.h>
#include <assert.h>
#include <string.h>
#include <ndk/sched.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>
#include <ndk/port.h>
#include <ndk/cpudata.h>
#include <ndk/vm.h>
#include <ndk/dpc.h>
#include <ndk/time.h>
#include <ndk/softint.h>

static kmem_cache_t *thread_cache = nullptr;

static spinlock_t done_lock;
static thread_queue_t done_queue;

extern void sched_port_switch(thread_t *old, thread_t *new);
extern void jumpstart(thread_t *new);
extern void port_initialize_context(context_t *context, int user, void *kstack,
				    thread_start_fn startfn, void *context1,
				    void *context2);

void done_queue_fn(dpc_t *, void *, void *)
{
	int old = port_set_ints(0);
	if (!spinlock_try_lock(&done_lock))
		return;
	while (!TAILQ_EMPTY(&done_queue)) {
		thread_t *elm = TAILQ_FIRST(&done_queue);
		TAILQ_REMOVE(&done_queue, elm, entry);
		// XXX: do thread destroy
	}
	spinlock_release(&done_lock);
	port_set_ints(old);
}

static inline scheduler_t *lsched()
{
	return &cpudata()->scheduler;
}

void port_initialize_context(context_t *context, int user, void *kstack,
			     thread_start_fn startfn, void *context1,
			     void *context2);

void preemption_dpc_fn(dpc_t *, void *, void *)
{
	sched_reschedule();
}

void sched_kmem_init()
{
	thread_cache = kmem_cache_create("thread_t", sizeof(thread_t), 0, NULL,
					 NULL, NULL, 0);
}

void sched_init(scheduler_t *sched)
{
	TAILQ_INIT(&done_queue);
	SPINLOCK_INIT(&done_lock);
	SPINLOCK_INIT(&sched->sched_lock);
	for (int i = 0; i < PRIORITY_COUNT; i++) {
		TAILQ_INIT(&sched->run_queues[i]);
	}

	dpc_init(&sched->preemption_dpc, preemption_dpc_fn);
	timer_init(&sched->preemption_timer);
}

thread_t *sched_alloc_thread()
{
	thread_t *thread = kmem_cache_alloc(thread_cache, 0);
	assert(thread);
	return thread;
}

void sched_init_thread(thread_t *thread, thread_start_fn ip, int priority,
		       void *context1, void *context2)
{
	memset(thread, 0x0, sizeof(thread_t));
	void *kstack_base = vm_kalloc(KSTACK_SIZE / PAGE_SIZE, 0);
	assert(kstack_base);
	void *kstack = (void *)((uintptr_t)kstack_base + KSTACK_SIZE);
	port_initialize_context(&thread->context, 0, kstack_base, ip, context1,
				context2);
	thread->state = kThreadStateNull;
	thread->priority = priority;
	thread->kstack_top = kstack;
	thread->timeslice = 30;
	thread->last_processor = cpudata()->cpu_id;
	thread->wait_count = -1;
	thread->wait_block_array = nullptr;
	thread->wait_status = -1;
	SPINLOCK_INIT(&thread->thread_lock);
}

void sched_destroy_thread(thread_t *thread)
{
	assert(!"todo");
}

static void check_timer(thread_t *thread, scheduler_t *sched)
{
	if (sched->run_mask >> thread->priority) {
		timer_uninstall(&sched->preemption_timer, 0);
		timer_set_in(&sched->preemption_timer, MS2NS(thread->timeslice),
			     &sched->preemption_dpc);
		timer_install(&sched->preemption_timer, NULL, NULL);
	} else {
		timer_uninstall(&sched->preemption_timer, 0);
	}
}

// sched lock held
// thread lock held
void sched_insert(scheduler_t *sched, thread_t *thread)
{
	assert(spinlock_held(&sched->sched_lock));
	assert(spinlock_held(&thread->thread_lock));
	assert(irql_current() >= IRQL_DISPATCH);
	thread_t *current, *next, *compare;

	thread->state = kThreadStateReady;

	current = cpudata()->thread_current;
	next = cpudata()->thread_next;

	compare = next;
	if (!compare)
		compare = current;

	// XXX: check if thread should preempt correctly

	if (compare->priority >= thread->priority) {
		printk("comp:%s t:%s\n", compare->name, thread->name);
		thread_queue_t *qp = &sched->run_queues[thread->priority];
		TAILQ_INSERT_TAIL(qp, thread, entry);
		sched->run_mask |= (1 << thread->priority);
		check_timer(compare, sched);
		return;
	}

	thread->state = kThreadStateStandby;

	// preempt
	cpudata()->thread_next = thread;
	if (next) {
		sched_insert(sched, thread);
	} else {
		softint_issue(IRQL_DISPATCH);
	}
}

static thread_t *sched_next(int priority)
{
	scheduler_t *sched = lsched();
	assert(spinlock_held(&sched->sched_lock));
	for (int i = PRIORITY_COUNT; i > priority; i--) {
		thread_queue_t *qp = &sched->run_queues[i - 1];
		if (TAILQ_EMPTY(qp))
			continue;
		thread_t *tp = TAILQ_FIRST(qp);
		TAILQ_REMOVE(qp, tp, entry);
		if (TAILQ_EMPTY(qp)) {
			sched->run_mask &= ~(1 << tp->priority);
		}
		return tp;
	}
	return nullptr;
}

void sched_reschedule()
{
	thread_t *current = cpudata()->thread_current;
	irql_t old = spinlock_acquire_raise(&lsched()->sched_lock);
	thread_t *next = sched_next(current->priority);
	assert(current);
	if (next != nullptr) {
		// current should be preempted
		cpudata()->thread_next = next;
	}
	spinlock_release_lower(&lsched()->sched_lock, old);
}

void sched_switch_thread(thread_t *current, thread_t *thread)
{
	cpudata()->thread_current = thread;
	timer_uninstall(&thread->wait_timer, 0);
	sched_port_switch(current, thread);
	// old thread lock is released after thread switch
}

// assumes:
// - dispatch irql
// - current thread lock held
void sched_preempt()
{
	//printk("%s called\n", __func__);
	cpudata_t *cpu = cpudata();
	scheduler_t *sched = lsched();

	spinlock_acquire(&sched->sched_lock);
	thread_t *next_thread = cpu->thread_next;
	cpu->thread_next = nullptr;
	spinlock_release(&sched->sched_lock);

	thread_t *current = cpu->thread_current;

	spinlock_acquire(&current->thread_lock);

	if (current != &cpu->idle_thread) {
		spinlock_acquire(&sched->sched_lock);
		//printk("insert current: %p\n", current);
		sched_insert(sched, current);
		spinlock_release(&sched->sched_lock);
	}

	sched_switch_thread(current, next_thread);
}

void sched_resume(thread_t *thread)
{
	if (thread->state == kThreadStateReady) {
		printk(ERR "THREAD STATE ALREADY READY\n");
		return;
	}
	scheduler_t *sched = lsched();
	irql_t ipl = spinlock_acquire_raise(&sched->sched_lock);
	spinlock_acquire(&thread->thread_lock);
	sched_insert(sched, thread);
	thread->last_processor = cpudata()->cpu_id;
	spinlock_release(&thread->thread_lock);
	spinlock_release_lower(&sched->sched_lock, ipl);
}

static void sched_yield_internal(thread_t *current)
{
	scheduler_t *sched = lsched();
	spinlock_acquire(&sched->sched_lock);
	thread_t *next = sched_next(0);
	spinlock_release(&sched->sched_lock);
	if (next == nullptr) {
		next = &cpudata()->idle_thread;
	}
	sched_switch_thread(current, next);
}

void sched_yield()
{
	thread_t *current = curthread();
	spinlock_acquire(&current->thread_lock);
	sched_yield_internal(current);
}

[[gnu::noreturn]] void sched_jump_into_idle_thread()
{
	thread_t *idlethread = &cpudata()->idle_thread;
	cpudata()->thread_current = idlethread;
	jumpstart(idlethread);
	__builtin_unreachable();
}

[[gnu::noreturn]] void sched_exit_destroy()
{
	thread_t *current = curthread();
	(void)irql_raise(IRQL_DISPATCH);
	spinlock_acquire(&done_lock);
	TAILQ_INSERT_TAIL(&done_queue, curthread(), entry);
	spinlock_acquire(&current->thread_lock);
	spinlock_release(&done_lock);
	sched_yield_internal(current);
	// this should never happen
	assert(!"yield returned to thread after exit_destroy\n");
}

thread_t *curthread()
{
	return cpudata()->thread_current;
}

void sched_sleep(uint64_t ns)
{
	timer_t timer;
	timer_init(&timer);
	timer_set_in(&timer, ns, nullptr);
	timer_install(&timer, nullptr, nullptr);
	assert(timer.hdr.waitercount == 0);
	assert(curthread()->wait_count == -1);
	sched_wait_single(&timer, kWaitTimeoutInfinite);
}
