#include <string.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>

void cpudata_setup(cpudata_t *cpudata)
{
	memset(cpudata, 0x0, sizeof(cpudata_t));
	cpudata->softint_pending = 0;
	TAILQ_INIT(&cpudata->dpc_queue.dpcq);
	cpudata_port_setup(&cpudata->port_data);
}

void __assert_fail(const char *assertion, const char *file, unsigned int line,
		   const char *function)
{
	printk(ERR "Assertion failure at %s:%d in function %s\nAssertion: %s\n",
	       file, line, function, assertion);
	panic("Assertion failure\n");
}
