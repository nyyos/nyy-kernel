#include <ndk/time.h>
#include <ndk/ndk.h>
#include <ndk/port.h>

clocksource_t *system_clocksource = nullptr;

clocksource_t *clocksource()
{
	return system_clocksource;
}

void register_clocksource(clocksource_t *csp)
{
	if (system_clocksource == nullptr ||
	    system_clocksource->priority < csp->priority) {
		system_clocksource = csp;
		printk(INFO "using clocksource %s\n", clocksource()->name);
	}
}

void time_delay(uint64_t ns)
{
	uint64_t deadline = clocksource()->current_nanos() + ns;
	while (clocksource()->current_nanos() < deadline)
		port_spin_hint();
}
