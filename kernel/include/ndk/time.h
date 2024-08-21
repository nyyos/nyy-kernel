#pragma once

#include <stdint.h>
#include <sys/queue.h>
#include <heap.h>
#include <ndk/ndk.h>

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

enum timer_state {
	kTimerUnused = 0,
	kTimerFired,
	kTimerQueued,
	kTimerCanceled
};

typedef struct timer {
	void (*callback)(void *private);
	void *private;

	uint64_t deadline;
	short timer_state;

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

timer_engine_t *gp_engine();
void set_gp_engine(timer_engine_t *ep);

void timer_install(timer_engine_t *ep, timer_t *tp);
void timer_uninstall(timer_t *tp);
void time_engine_update(timer_engine_t *ep);
void timer_destroy(timer_t *tp);

timer_t *timer_create(timer_t *tp, uint64_t deadline, void (*callback)(void *),
		      void *private);
