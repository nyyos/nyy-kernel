#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <string.h>

spinlock_t g_pac_lock;

irql_t spinlock_acquire(spinlock_t *spinlock, irql_t at)
{
	int unlocked = 0;
	while (!__atomic_compare_exchange_n(&spinlock->flag, &unlocked, 1, 0,
					    __ATOMIC_ACQUIRE,
					    __ATOMIC_RELAXED)) {
		unlocked = 0;
		port_spin_hint();
	}
	return irql_raise(at);
}

void spinlock_release(spinlock_t *spinlock, irql_t to)
{
	__atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELEASE);
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
	printk(ERR "Assertion failure at %s:%d in function %s\nAssertion: %s\n",
	       file, line, function, assertion);
	panic("Assertion failure\n");
}
