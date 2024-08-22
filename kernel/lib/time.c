#include <string.h>
#include <heap.h>
#include <ndk/time.h>
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/kmem.h>

static kmem_cache_t *g_timer_cache;

static int timer_cmp(timer_t *a, timer_t *b)
{
	return a->deadline < b->deadline;
}

HEAP_IMPL(timerlist, timer, entry, timer_cmp);

clocksource_t *system_clocksource = nullptr;
timer_engine_t *_gp_engine;

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

void time_init()
{
	g_timer_cache = kmem_cache_create("timer", sizeof(timer_t), 0, NULL,
					  NULL, NULL, 0);
}

timer_t *timer_create(timer_t *tp, uint64_t deadline, void (*callback)(void *),
		      void *private)
{
	if (tp == NULL) {
		tp = kmem_cache_alloc(g_timer_cache, 0);
	}
	tp->engine = NULL;
	tp->deadline = deadline;
	tp->callback = callback;
	tp->private = private;
	tp->timer_state = kTimerUnused;
	return tp;
}

void timer_destroy(timer_t *tp)
{
	irql_t irql;

	if (tp->engine != NULL)
		irql = spinlock_acquire(&tp->engine->lock, IRQL_HIGH);

	assert(tp->timer_state == kTimerUnused ||
	       tp->timer_state == kTimerFired);

	kmem_cache_free(g_timer_cache, tp);

	if (tp->engine != NULL)
		spinlock_release(&tp->engine->lock, irql);

	tp->engine = NULL;
}

static void time_engine_progress(timer_engine_t *ep)
{
	clocksource_t *cp = ep->clocksource;
	uint64_t now = cp->current_nanos();

	do {
		while (1) {
			if (HEAP_EMPTY(&ep->timerlist)) {
				ep->arm(0);
				return;
			}

			if (HEAP_PEEK(&ep->timerlist)->deadline > now)
				break;

			timer_t *timer = HEAP_POP(timerlist, &ep->timerlist);
			assert(timer);
			timer->timer_state = kTimerFired;
			timer->callback(timer->private);
			timer->engine = NULL;
		}
		ep->arm(HEAP_PEEK(&ep->timerlist)->deadline);
		now = cp->current_nanos();
	} while (HEAP_PEEK(&ep->timerlist)->deadline <= now);
}

void timer_uninstall(timer_t *tp)
{
	timer_engine_t *ep = tp->engine;
	irql_t irql = spinlock_acquire(&ep->lock, IRQL_HIGH);
	tp->engine = NULL;
	if (tp->timer_state == kTimerQueued)
		tp->timer_state = kTimerCanceled;
	else
		tp->timer_state = kTimerUnused;
	HEAP_REMOVE(timerlist, &ep->timerlist, tp);
	time_engine_progress(ep);
	spinlock_release(&ep->lock, irql);
}

void timer_install(timer_engine_t *ep, timer_t *tp)
{
	assert(ep);
	assert(tp->timer_state != kTimerQueued);
	irql_t irql = spinlock_acquire(&ep->lock, IRQL_HIGH);
	tp->engine = ep;
	tp->timer_state = kTimerQueued;
	HEAP_INSERT(timerlist, &ep->timerlist, tp);
	time_engine_progress(ep);
	spinlock_release(&ep->lock, irql);
}

void time_engine_update(timer_engine_t *ep)
{
	assert(ep);
	irql_t irql = spinlock_acquire(&ep->lock, IRQL_HIGH);
	time_engine_progress(ep);
	spinlock_release(&ep->lock, irql);
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

timer_engine_t *gp_engine()
{
	return _gp_engine;
}

void set_gp_engine(timer_engine_t *ep)
{
	assert(ep);
	_gp_engine = ep;
}
