#include <ndk/int.h>
#include <ndk/ndk.h>
#include <ndk/dpc.h>
#include <ndk/cpudata.h>

#define PENDING(irql) ((1 << ((irql) - 1)))

void softint_dispatch(irql_t newirql)
{
	int old = port_set_ints(0);
	while ((cpudata()->softint_pending & (0xff << newirql)) != 0) {
		if (cpudata()->softint_pending & PENDING(IRQL_DISPATCH)) {
			softint_ack(IRQL_DISPATCH);
			irql_set(IRQL_DISPATCH);
			port_enable_ints();
			dpc_run_queue();
			port_disable_ints();
		}
	}

	irql_set(newirql);

	port_set_ints(old);
}

void softint_issue(irql_t irql)
{
	__atomic_or_fetch(&cpudata()->softint_pending, PENDING(irql),
			  __ATOMIC_ACQUIRE);
}

void softint_ack(irql_t irql)
{
	__atomic_and_fetch(&cpudata()->softint_pending, ~PENDING(irql),
			   __ATOMIC_RELEASE);
}
