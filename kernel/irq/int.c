#include <ndk/int.h>
#include <ndk/ndk.h>
#include <ndk/dpc.h>
#include <ndk/cpudata.h>

#define PENDING(irql) ((1 << ((irql) - 1)))

void softint_dispatch(irql_t newirql)
{
	while ((cpudata()->softint_pending & (0xff << newirql)) != 0) {
		if (cpudata()->softint_pending & PENDING(IRQL_DISPATCH)) {
			dpc_run_queue();
		}
	}
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
