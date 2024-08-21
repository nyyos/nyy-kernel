#pragma once

#include <stdint.h>
#include <sys/queue.h>

#define NS2FEMTOS(X) ((X) * 1000000L)
#define FEMTOS2NS(X) ((X) / 1000000L)
#define US2NS(X) ((X) * 1000L)
#define MS2NS(X) (US2NS((X) * 1000L))

typedef struct clocksource {
	const char *name;
	int priority;

	uint64_t (*current_nanos)();
} clocksource_t;

// return currently active systemclocksource
clocksource_t *clocksource();
void register_clocksource(clocksource_t *csp);

void time_delay(uint64_t ns);
