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
#include <ndk/int.h>

static kmem_cache_t *thread_cache = nullptr;

static spinlock_t done_lock;
static thread_queue_t done_queue;

static void done_queue_dpc(dpc_t *, void *, void *)
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

extern void asm_switch_context(thread_t *new, thread_t *old);
extern void asm_jumpstart(thread_t *new);

void port_initialize_context(context_t *context, int user, void *kstack,
			     thread_start_fn startfn, void *context1,
			     void *context2);

static void dpc_reschedule_interval(dpc_t *, void *, void *)
{
	sched_reschedule();
}

void sched_kmem_init()
{
	thread_cache = kmem_cache_create("thread_t", sizeof(thread_t), 0, NULL,
					 NULL, NULL, 0);
}

void sched_init()
{
	TAILQ_INIT(&done_queue);

	scheduler_t *lsp = lsched();
	SPINLOCK_INIT(&lsp->sched_lock);
	for (int i = 0; i < PRIORITY_COUNT; i++) {
		TAILQ_INIT(&lsp->run_queues[i]);
	}

	dpc_init(&lsp->reschedule_dpc_timer, dpc_reschedule_interval);
	dpc_init(&cpudata()->done_queue_dpc, done_queue_dpc);
}

extern void thread_trampoline();

void port_initialize_context(context_t *context, int user, void *kstack,
			     thread_start_fn startfn, void *context1,
			     void *context2)
{
	context->rflags = 0x200;
	if (user != 0) {
		context->cs = kGdtUserCode * 8;
		context->ss = kGdtUserData * 8;
	} else {
		context->cs = kGdtKernelCode * 8;
		context->ss = kGdtKernelData * 8;
	}

	uint64_t *sp = (uint64_t *)((uintptr_t)kstack + KSTACK_SIZE);
	sp -= 2;
	*sp = (uint64_t)thread_trampoline;
	context->rsp = (uint64_t)sp;
	context->r12 = (uint64_t)startfn;
	context->r13 = (uint64_t)context1;
	context->r14 = (uint64_t)context2;
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
	SPINLOCK_INIT(&thread->thread_lock);
}

void sched_destroy_thread(thread_t *thread)
{
	assert(!"todo");
}

// sched lock held
// thread lock held
static void sched_insert(scheduler_t *sched, thread_t *thread)
{
	thread_t *current, *next, *compare;
	current = cpudata()->thread_current;
	next = cpudata()->thread_next;

	compare = next;
	if (!compare)
		compare = current;

	// XXX: check if thread should preempt correctly

	if (compare != &cpudata()->idle_thread) {
		compare->state = kThreadStateReady;
		thread_queue_t *qp = &sched->run_queues[thread->priority];
		TAILQ_INSERT_TAIL(qp, thread, entry);
		return;
	}

	// preempt
	cpudata()->thread_next = thread;
	if (next)
		sched_insert(sched, thread);
	else
		softint_issue(IRQL_DISPATCH);
}

static thread_t *sched_next(int priority)
{
	scheduler_t *sched = lsched();
#if 0
	printk("queue state:");
	for (int i = 0; i < PRIORITY_COUNT; i++) {
		thread_queue_t *qp = &sched->run_queues[i];
		printk("%d", TAILQ_EMPTY(qp) ? 0 : 1);
	}
	printk("\n");
#endif
	for (int i = PRIORITY_COUNT; i >= priority; i--) {
		thread_queue_t *qp = &sched->run_queues[i - 1];
		if (TAILQ_EMPTY(qp))
			continue;
		thread_t *tp = TAILQ_FIRST(qp);
		TAILQ_REMOVE(qp, tp, entry);
		return tp;
	}
	return nullptr;
}

void sched_port_switch(thread_t *old, thread_t *new)
{
	asm_switch_context(new, old);
}

void sched_reschedule()
{
	thread_t *current = cpudata()->thread_current;
	irql_t old = spinlock_acquire_raise(&lsched()->sched_lock);
	thread_t *next = sched_next(current->priority);
	spinlock_release_lower(&lsched()->sched_lock, old);
	assert(current);
	if (next != nullptr) {
		// current should be preempted
		cpudata()->thread_next = next;
	}
}

void sched_switch_thread(thread_t *current, thread_t *thread)
{
	cpudata()->thread_current = thread;
	sched_port_switch(current, thread);
	// old thread lock is released after thread switch
}

// assumes:
// - dispatch irql
// - current thread lock held
void sched_preempt()
{
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
		sched_insert(sched, current);
		spinlock_release(&sched->sched_lock);
	}

	sched_switch_thread(current, next_thread);
}

void sched_resume(thread_t *thread)
{
	irql_t old = spinlock_acquire_raise(&thread->thread_lock);
	scheduler_t *sched = lsched();
	spinlock_acquire(&sched->sched_lock);
	sched_insert(sched, thread);
	spinlock_release(&sched->sched_lock);
	spinlock_release_lower(&thread->thread_lock, old);
}

void sched_yield(thread_t *current)
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

[[gnu::noreturn]] void sched_jump_into_idle_thread()
{
	thread_t *idlethread = &cpudata()->idle_thread;
	cpudata()->thread_current = idlethread;
	asm_jumpstart(idlethread);
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
	sched_yield(current);
	// this should never happen
	assert(!"yield returned to thread after exit_destroy\n");
}

thread_t *curthread()
{
	return cpudata()->thread_current;
}
