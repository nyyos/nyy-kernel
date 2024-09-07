#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <heap.h>
#include <ndk/ndk.h>
#include <ndk/dpc.h>

#define NS2FEMTOS(X) ((X) * 1000000L)
#define NS2US(X) ((X) / 1000L)
#define NS2MS(X) (NS2US(X) / 1000L)

#define FEMTOS2NS(X) ((X) / 1000000L)
#define US2NS(X) ((X) * 1000L)
#define MS2NS(X) (US2NS((X) * 1000L))

typedef struct clocksource {
	const char *name;
	int priority;

	uint64_t (*current_nanos)();
} clocksource_t;

typedef struct timer_engine timer_engine_t;

enum timer_management_flags {
	kTimerEngineLockHeld = (1 << 1),
};

enum timer_state {
	kTimerUnused = 0,
	kTimerFired,
	kTimerQueued,
	kTimerCanceled
};

enum timer_mode {
	kTimerOneshotMode = 0,
	kTimerPeriodicMode,
};

typedef struct timer {
	dpc_t *dpc;

	uint64_t deadline;
	uint64_t interval; // used by periodic mode
	short timer_state;

	int mode;

	timer_engine_t *engine;
	HEAP_ENTRY(timer) entry;
} timer_t;

typedef struct timer_engine {
	clocksource_t *clocksource;
	void (*arm)(uint64_t deadline);

	spinlock_t lock;
	HEAP_HEAD(timerlist, timer) timerlist;
} timer_engine_t;

// return currently active systemclocksource
clocksource_t *clocksource();
void register_clocksource(clocksource_t *csp);

void time_delay(uint64_t ns);

// has to be called before
// any other time related thing is used
void time_init();
void time_engine_init(timer_engine_t *ep, clocksource_t *csp,
		      void (*arm)(uint64_t));

timer_t *timer_allocate();
void timer_free(timer_t *timer);

void timer_initialize(timer_t *tp, uint64_t deadline, dpc_t *dpc, int mode);
void timer_reset(timer_t *tp, uint64_t deadline);
void timer_reset_in(timer_t *tp, uint64_t in_ns);

void timer_install(timer_t *tp, void *context1, void *context2);
void timer_uninstall(timer_t *tp, int flags);
