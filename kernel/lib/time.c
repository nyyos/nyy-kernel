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

static clocksource_t *system_clocksource = nullptr;
static timer_engine_t *system_gp_engine = nullptr;

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

timer_t *timer_allocate()
{
	return kmem_cache_alloc(g_timer_cache, 0);
}

void timer_free(timer_t *tp)
{
	irql_t irql;

	if (tp->engine != NULL)
		irql = spinlock_acquire(&tp->engine->lock, IRQL_CLOCK);

	assert(tp->timer_state == kTimerQueued &&
		       tp->mode == kTimerPeriodicMode ||
	       tp->timer_state == kTimerUnused ||
	       tp->timer_state == kTimerFired);

	kmem_cache_free(g_timer_cache, tp);

	if (tp->engine != NULL)
		spinlock_release(&tp->engine->lock, irql);

	tp->engine = NULL;
}

void timer_set(timer_t *tp, uint64_t deadline, void (*callback)(void *),
	       void *private, int mode)
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
	tp->callback = callback;
	tp->private = private;
	tp->mode = mode;
	tp->timer_state = kTimerUnused;
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
			if (timer->mode == kTimerPeriodicMode) {
				timer->timer_state = kTimerFired;
				timer->callback(timer->private);
				if (timer->engine != NULL) {
					timer->deadline = now + timer->interval;
					timer->timer_state = kTimerQueued;
					HEAP_INSERT(timerlist, &ep->timerlist,
						    timer);
				}
			} else {
				timer->timer_state = kTimerFired;
				timer->engine = NULL;
				timer->callback(timer->private);
			}
		}
		ep->arm(HEAP_PEEK(&ep->timerlist)->deadline);
		now = cp->current_nanos();
	} while (HEAP_PEEK(&ep->timerlist)->deadline <= now);
}

void timer_uninstall(timer_t *tp, int flags)
{
	if (flags & kTimerEngineLockHeld) {
		tp->engine = NULL;
		return;
	}
	timer_engine_t *ep = tp->engine;
	irql_t irql = spinlock_acquire(&ep->lock, IRQL_CLOCK);
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
	irql_t irql = spinlock_acquire(&ep->lock, IRQL_CLOCK);
	tp->engine = ep;
	tp->timer_state = kTimerQueued;
	if (tp->mode == kTimerPeriodicMode)
		tp->deadline = ep->clocksource->current_nanos() + tp->interval;
	HEAP_INSERT(timerlist, &ep->timerlist, tp);
	time_engine_progress(ep);
	spinlock_release(&ep->lock, irql);
}

void time_engine_update(timer_engine_t *ep)
{
	assert(ep);
	irql_t irql = spinlock_acquire(&ep->lock, IRQL_CLOCK);
	//printk("update ep lock\n");
	time_engine_progress(ep);
	spinlock_release(&ep->lock, irql);
	//printk("update ep unlock\n");
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
	return system_gp_engine;
}

void set_gp_engine(timer_engine_t *ep)
{
	assert(ep);
	system_gp_engine = ep;
}
