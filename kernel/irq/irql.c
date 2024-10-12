#include <assert.h>
#include <ndk/irql.h>
#include <ndk/cpudata.h>
#include <ndk/softint.h>

irql_t irql_raise(irql_t level)
{
	irql_t old = irql_current();
	assert(level >= old);
	irql_set(level);
	return old;
}

void irql_lower(irql_t level)
{
	irql_t cur = irql_current();
	assert(level <= cur);
	irql_set(level);
	if (cpudata()->softint_pending >> level) {
		softint_dispatch(level);
	}
}
