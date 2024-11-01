#include <string.h>
#include <ndk/time.h>
#include <ndk/ndk.h>
#include <ndk/port.h>
#include <ndk/kmem.h>
#include <ndk/dpc.h>
#include <ndk/cpudata.h>
#include <sys/tree.h>
#include <assert.h>
#include <heap.h>

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

	assert(tp->timer_state == kTimerUnused ||
	       tp->timer_state == kTimerFired);

	kmem_cache_free(g_timer_cache, tp);

	if (tp->engine != NULL)
		spinlock_release_lower(&tp->engine->lock, irql);

	tp->engine = NULL;
}

void timer_init(timer_t *tp)
{
	assert(tp);
	assert(tp->timer_state != kTimerQueued);
	obj_init(tp, kObjTypeAnon);
	tp->engine = NULL;
	tp->deadline = -1;
	tp->timer_state = kTimerUnused;

	HEAP_CHILD(tp, entry) = nullptr;
	HEAP_NEXT(tp, entry) = nullptr;
	HEAP_PREV(tp, entry) = nullptr;
}

void timer_set(timer_t *tp, uint64_t deadline, dpc_t *dpc)
{
	assert(tp);
	irql_t irql = spinlock_acquire_raise(&tp->hdr.object_lock);

	if (tp->timer_state == kTimerQueued) {
		timer_uninstall(tp, 0);
	}
	tp->dpc = dpc;
	tp->deadline = deadline;

	spinlock_release_lower(&tp->hdr.object_lock, irql);
}

void timer_set_in(timer_t *tp, uint64_t in_ns, dpc_t *dpc)
{
	timer_set(tp, clocksource()->current_nanos() + in_ns, dpc);
}

static void time_engine_progress(dpc_t *dpc, void *context1, void *context2)
{
	timer_engine_t *ep = &cpudata()->timer_engine;
	clocksource_t *cp = ep->clocksource;
	uint64_t now;

	while (1) {
		now = cp->current_nanos();
		assert(irql_current() == IRQL_DISPATCH);
		spinlock_acquire(&ep->lock);

		if (HEAP_EMPTY(&ep->timerlist)) {
			cpudata()->next_deadline = UINT64_MAX;
			ep->arm(0);
			spinlock_release(&ep->lock);
			return;
		}

		timer_t *root = HEAP_PEEK(&ep->timerlist);

		if (root->deadline > now) {
			uint64_t next = root->deadline;
			cpudata()->next_deadline = next;
			ep->arm(next);
			spinlock_release(&ep->lock);
			break;
		}

		root = HEAP_POP(timerlist, &ep->timerlist);
		spinlock_acquire(&root->hdr.object_lock);
		root->engine = nullptr;

		spinlock_release(&ep->lock);

		root->timer_state = kTimerFired;

		if (root->hdr.waitercount) {
			obj_satisfy(&root->hdr, true, 0);
			root->hdr.signalcount = 1;
		}

		dpc_t *timerdpc = root->dpc;

		if (timerdpc) {
			context1 = timerdpc->context1;
			context2 = timerdpc->context2;
		}

		spinlock_release(&root->hdr.object_lock);

		if (timerdpc) {
			timerdpc->function(timerdpc, context1, context2);
		}
	}
}

void timer_uninstall(timer_t *tp, int flags)
{
	timer_engine_t *ep = tp->engine;
	if (tp->engine != nullptr) {
		irql_t irql = spinlock_acquire_raise(&ep->lock);
		spinlock_acquire(&tp->hdr.object_lock);

		HEAP_REMOVE(timerlist, &ep->timerlist, tp);

		tp->engine = nullptr;

		tp->timer_state = kTimerCanceled;

		dpc_enqueue(&cpudata()->timer_update, NULL, NULL);

		spinlock_release(&tp->hdr.object_lock);
		spinlock_release_lower(&ep->lock, irql);
	} else {
		irql_t irql = spinlock_acquire_raise(&tp->hdr.object_lock);

		tp->timer_state = kTimerCanceled;

		spinlock_release_lower(&tp->hdr.object_lock, irql);
	}
}

void timer_install(timer_t *tp, void *context1, void *context2)
{
	assert(tp->timer_state != kTimerQueued);
	timer_engine_t *ep = &cpudata()->timer_engine;
	irql_t irql = spinlock_acquire_raise(&ep->lock);
	spinlock_acquire(&tp->hdr.object_lock);
	if (tp->engine) {
		spinlock_release(&tp->hdr.object_lock);
		spinlock_release_lower(&ep->lock, irql);
		return;
	}

	tp->engine = ep;
	tp->timer_state = kTimerQueued;

	if (tp->dpc) {
		tp->dpc->context1 = context1;
		tp->dpc->context2 = context2;
	}

	tp->hdr.signalcount = 0;

	HEAP_INSERT(timerlist, &ep->timerlist, tp);

	dpc_enqueue(&cpudata()->timer_update, NULL, NULL);

	spinlock_release(&tp->hdr.object_lock);
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
