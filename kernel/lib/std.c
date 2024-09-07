#include <string.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <ndk/sched.h>

extern void done_queue_fn(dpc_t *, void *, void *);

static volatile int next_cpu_id = 0;

void cpudata_setup(cpudata_t *cpudata)
{
	memset(cpudata, 0x0, sizeof(cpudata_t));
	cpudata->cpu_id = __atomic_fetch_add(&next_cpu_id, 1, __ATOMIC_RELAXED);
	cpudata->softint_pending = 0;
	TAILQ_INIT(&cpudata->dpc_queue.dpcq);
	sched_init(&cpudata->scheduler);
	cpudata->thread_next = nullptr;
	cpudata->thread_current = nullptr;
	dpc_init(&cpudata->done_queue_dpc, done_queue_fn);

	cpudata->next_deadline = UINT64_MAX;
	cpudata_port_setup(&cpudata->port_data);
}

void __assert_fail(const char *assertion, const char *file, unsigned int line,
		   const char *function)
{
	printk(ERR "Assertion failure at %s:%d in function %s\nAssertion: %s\n",
	       file, line, function, assertion);
	panic("Assertion failure\n");
}
