#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <stdatomic.h>
#include <string.h>

spinlock_t g_pac_lock;

irql_t spinlock_acquire(spinlock_t *spinlock, irql_t at)
{
	while (atomic_flag_test_and_set(&spinlock->flag)) {
#ifdef __x86_64__
		asm volatile("pause");
#endif
	}
	return irql_raise(at);
}

void spinlock_release(spinlock_t *spinlock, irql_t to)
{
	atomic_flag_clear(&spinlock->flag);
	irql_lower(to);
}

void cpudata_setup(cpudata_t *cpudata)
{
	memset(cpudata, 0x0, sizeof(cpudata_t));
	cpudata_port_setup(&cpudata->port_data);
}

void __assert_fail(const char *assertion, const char *file, unsigned int line,
		   const char *function)
{
	char buf[512];
	npf_snprintf(buf, sizeof(buf), "Assertion failure at %s:%d in function %s\nAssertion: %s\n", file, line, function, assertion);
	panic(buf);
}
