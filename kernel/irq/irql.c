#include <assert.h>
#include <ndk/irql.h>

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
}
