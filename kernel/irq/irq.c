#include <sys/queue.h>
#include <assert.h>
#include <ndk/irq.h>
#include <ndk/kmem.h>
#include <ndk/irql.h>
#include <ndk/port.h>
#include <ndk/ndk.h>
#include <ndk/cpudata.h>

static kmem_cache_t *irq_obj_cache;
static spinlock_t s_irq_lock;
static irq_vector_t s_vectors[IRQ_VECTOR_MAX];

void irq_init()
{
	irq_obj_cache = kmem_cache_create("irq_obj", sizeof(irq_t), 0, NULL,
					  NULL, NULL, 0);
	for (int i = 0; i < IRQ_VECTOR_MAX; i++) {
		irq_vector_t *handler = &s_vectors[i];
		handler->vector = i;
		handler->objectcnt = 0;
		handler->flags = 0;
		TAILQ_INIT(&handler->objq);
	}
}

irq_t *irq_allocate_obj()
{
	irq_t *obj = kmem_cache_alloc(irq_obj_cache, 0);
	obj->vector = nullptr;
	return obj;
}

void irq_free_obj(irq_t *obj)
{
	kmem_cache_free(irq_obj_cache, obj);
}

static void irq_do_dispatch(interrupt_frame_t *iframe, vector_t vec)
{
	irq_t *obj;
	irq_vector_t *handler = &s_vectors[vec];
	if (handler->objectcnt == 0) {
		goto unhandled;
	}

	size_t ack_count = 0;
	TAILQ_FOREACH(obj, &handler->objq, entry)
	{
		if (obj->handler) {
			if (obj->handler(obj, iframe, obj->private) == kIrqAck)
				ack_count++;
		}
	}

	if (ack_count == 0)
		goto unhandled;

	return;

unhandled:
	// XXX: we could do some masking for t time here, so we dont
	// use up cpu time cause of a broken device, but ensure future functionality
	// I think I saw this in managarm
	printk(ERR "vector is unhandled (%d)\n", vec);
}

static int irq_register_handler(irq_t *obj, vector_t vec, int flags)
{
	irq_vector_t *handler = &s_vectors[vec];
	if (handler->objectcnt == 0) {
		handler->flags = flags;
	}
	TAILQ_INSERT_TAIL(&handler->objq, obj, entry);
	handler->objectcnt++;
	obj->vector = handler;
	return port_register_isr(vec, irq_do_dispatch);
}

int irq_initialize_obj_irql(irq_t *obj, irql_t irqlWanted, int flags)
{
	assert(obj);
	assert(obj->vector == nullptr);
	if (flags & IRQ_FORCE)
		printk(DEBUG "IRQ_FORCE currently ignored\n");

	irql_t oldIrql = spinlock_acquire(&s_irq_lock, IRQL_HIGH);

	vector_t search_base = IRQL_TO_VECTOR(irqlWanted);
	vector_t found = IRQ_VECTOR_MAX;

	for (vector_t vec = search_base;
	     vec < search_base + IRQL_VECTOR_COUNT_PER; vec++) {
		irq_vector_t *handler = &s_vectors[vec];
		if (handler->objectcnt == 0) {
			found = vec;
			break;
		}
		if ((handler->flags & IRQ_SHAREABLE) &&
		    (flags & IRQ_SHAREABLE)) {
			if (found != IRQ_VECTOR_MAX) {
				if (s_vectors[found].objectcnt >
				    handler->objectcnt)
					found = vec;
			} else {
				found = vec;
			}
		}
	}

	if (found == IRQ_VECTOR_MAX) {
		spinlock_release(&s_irq_lock, oldIrql);
		return 1;
	}

	int ret = irq_register_handler(obj, found, flags);

	spinlock_release(&s_irq_lock, oldIrql);
	return ret;
}
