#include <sys/queue.h>
#include <assert.h>
#include <string.h>
#include <ndk/cpudata.h>
#include <ndk/sched.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>
#include <ndk/port.h>
#include <ndk/vm.h>

static kmem_cache_t *task_cache = nullptr;

static inline scheduler_t *lsched()
{
	return &cpudata()->scheduler;
}

void sched_init()
{
	if (task_cache == nullptr)
		task_cache = kmem_cache_create("task_cache", sizeof(task_t), 0,
					       NULL, NULL, NULL, 0);

	scheduler_t *lsp = lsched();
	SPINLOCK_INIT(&lsp->sched_lock);
	for (int i = 0; i < PRIORITY_COUNT; i++) {
		TAILQ_INIT(&lsp->task_queues[i]);
	}
}

extern void thread_trampoline();

void port_initialize_context(int user, void *kstack, task_start_fn startfn,
			     void *context1, void *context2)
{
	context_t *context =
		(context_t *)((uintptr_t)kstack - sizeof(context_t));

	context->rflags = 0x200;
	if (user != 0) {
		context->cs = kGdtUserCode * 8;
		context->ss = kGdtUserData * 8;
	} else {
		context->cs = kGdtKernelCode * 8;
		context->ss = kGdtKernelData * 8;
	}

	context->rsp = (uint64_t)context;
	context->rip = (uint64_t)thread_trampoline;
	context->r12 = (uint64_t)startfn;
	context->r13 = (uint64_t)context1;
	context->r14 = (uint64_t)context2;
}

task_t *sched_create_task(task_start_fn ip, int priority, void *context1,
			  void *context2)
{
	task_t *task = kmem_cache_alloc(task_cache, 0);
	memset(task, 0x0, sizeof(task_t));
	void *kstack_base = vm_kalloc(4, 0);
	void *kstack = (void *)((uintptr_t)kstack_base + 4 * PAGE_SIZE -
				sizeof(context_t));
	context_t *context = (context_t *)kstack;
	task->context = context;
	port_init_state(task->context, 0);
	STATE_SP(task->context) = (uintptr_t)kstack;
	STATE_IP(task->context) = (uintptr_t)thread_trampoline;
	STATE_ARG1(task->context) = (uintptr_t)ip;
	STATE_ARG2(task->context) = (uintptr_t)context1;
	STATE_ARG3(task->context) = (uintptr_t)context2;
	task->priority = priority;
	SPINLOCK_INIT(&task->task_lock);
	return task;
}

void sched_destroy_task(task_t *task)
{
	assert(!"todo");
}

void sched_preempt()
{
	assert(!"todo");
}
