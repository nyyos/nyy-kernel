#include <ndk/sched.h>

extern void thread_trampoline();
extern void switch_context(thread_t *new, thread_t *old);

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

void sched_port_switch(thread_t *old, thread_t *new)
{
	switch_context(new, old);
}
