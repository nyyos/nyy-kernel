#include <string.h>
#include <heap.h>
#include <ndk/time.h>
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/kmem.h>
#include <ndk/dpc.h>
#include <ndk/cpudata.h>

static kmem_cache_t *g_timer_cache;

static int timer_cmp(timer_t *a, timer_t *b)
{
	return a->deadline < b->deadline;
}

HEAP_IMPL(timerlist, timer, entry, timer_cmp);

static clocksource_t *system_clocksource = nullptr;

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

static void time_engine_progress(dpc_t *dpc, void *context1, void *context2);

void time_init()
{
	g_timer_cache = kmem_cache_create("timer", sizeof(timer_t), 0, NULL,
					  NULL, NULL, 0);
	dpc_init(&cpudata()->timer_update, time_engine_progress);
}

timer_t *timer_allocate()
{
	return kmem_cache_alloc(g_timer_cache, 0);
}

void timer_free(timer_t *tp)
{
	irql_t irql;

	if (tp->engine != NULL)
		irql = spinlock_acquire_raise(&tp->engine->lock);

	assert(tp->timer_state == kTimerQueued &&
		       tp->mode == kTimerPeriodicMode ||
	       tp->timer_state == kTimerUnused ||
	       tp->timer_state == kTimerFired);

	kmem_cache_free(g_timer_cache, tp);

	if (tp->engine != NULL)
		spinlock_release_lower(&tp->engine->lock, irql);

	tp->engine = NULL;
}

void timer_initialize(timer_t *tp, uint64_t deadline, dpc_t *dpc, int mode)
{
	assert(tp);
	assert(tp->timer_state != kTimerQueued);
	tp->engine = NULL;
	if (mode == kTimerPeriodicMode) {
		tp->interval = deadline;
		tp->deadline = -1;
	} else {
		tp->interval = -1;
		tp->deadline = deadline;
	}
	tp->dpc = dpc;
	tp->mode = mode;
	tp->timer_state = kTimerUnused;
}

void timer_reset(timer_t *tp, uint64_t deadline)
{
	assert(tp);
	assert(tp->mode == kTimerOneshotMode);
	if (tp->timer_state == kTimerQueued) {
		timer_uninstall(tp, 0);
	}
	tp->deadline = deadline;
}

void timer_reset_in(timer_t *tp, uint64_t in_ns)
{
	timer_reset(tp, clocksource()->current_nanos() + in_ns);
}

static void time_engine_progress(dpc_t *dpc, void *context1, void *context2)
{
	timer_engine_t *ep = &cpudata()->timer_engine;
	clocksource_t *cp = ep->clocksource;
	uint64_t now;

	while (1) {
		now = cp->current_nanos();
		irql_t old = spinlock_acquire_raise(&ep->lock);
		if (HEAP_EMPTY(&ep->timerlist)) {
			cpudata()->next_deadline = UINT64_MAX;
			ep->arm(0);
			spinlock_release_lower(&ep->lock, old);
			return;
		}

		if (HEAP_PEEK(&ep->timerlist)->deadline > now) {
			uint64_t next = HEAP_PEEK(&ep->timerlist)->deadline;
			cpudata()->next_deadline = next;
			ep->arm(next);
			spinlock_release_lower(&ep->lock, old);
			break;
		}

		timer_t *timer = HEAP_POP(timerlist, &ep->timerlist);

		if (timer->mode != kTimerPeriodicMode)
			timer->engine = nullptr;

		spinlock_release_lower(&ep->lock, old);

		timer->timer_state = kTimerFired;
		dpc_t *timerdpc = timer->dpc;

		if (timerdpc) {
			context1 = timerdpc->context1;
			context2 = timerdpc->context2;
			timerdpc->function(timerdpc, context1, context2);
		}

		if (timer->mode == kTimerPeriodicMode) {
			if (timer->engine != NULL) {
				timer->deadline = now + timer->interval;
				irql_t old = spinlock_acquire_raise(&ep->lock);
				timer->timer_state = kTimerQueued;
				HEAP_INSERT(timerlist, &ep->timerlist, timer);
				spinlock_release_lower(&ep->lock, old);
			}
		}
	}
}

void timer_uninstall(timer_t *tp, int flags)
{
	if (flags & kTimerEngineLockHeld) {
		tp->engine = NULL;
		return;
	}
	timer_engine_t *ep = tp->engine;
	irql_t irql = spinlock_acquire_raise(&ep->lock);
	tp->engine = NULL;
	if (tp->timer_state == kTimerQueued)
		tp->timer_state = kTimerCanceled;
	else
		tp->timer_state = kTimerUnused;
	HEAP_REMOVE(timerlist, &ep->timerlist, tp);
	dpc_enqueue(&cpudata()->timer_update, NULL, NULL);
	spinlock_release_lower(&ep->lock, irql);
}

void timer_install(timer_t *tp, void *context1, void *context2)
{
	assert(tp->timer_state != kTimerQueued);
	timer_engine_t *ep = &cpudata()->timer_engine;
	irql_t irql = spinlock_acquire_raise(&ep->lock);
	tp->engine = ep;
	tp->timer_state = kTimerQueued;
	if (tp->mode == kTimerPeriodicMode)
		tp->deadline = ep->clocksource->current_nanos() + tp->interval;
	if (tp->dpc) {
		tp->dpc->context1 = context1;
		tp->dpc->context2 = context2;
	}
	HEAP_INSERT(timerlist, &ep->timerlist, tp);
	dpc_enqueue(&cpudata()->timer_update, NULL, NULL);
	spinlock_release_lower(&ep->lock, irql);
}

void time_engine_init(timer_engine_t *ep, clocksource_t *csp,
		      void (*arm)(uint64_t))
{
	assert(ep);
	ep->clocksource = csp;
	ep->arm = arm;
	SPINLOCK_INIT(&ep->lock);
	HEAP_INIT(&ep->timerlist);
}
