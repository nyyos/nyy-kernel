#pragma once

#include <sys/queue.h>
#include <ndk/irql.h>
#include <ndk/port.h>

#define IRQ_SHAREABLE (1 << 0)
#define IRQ_FORCE (1 << 1)

typedef unsigned int vector_t;
typedef struct irq irq_t;
typedef void (*irq_handler_fn_t)(interrupt_frame_t *frame, vector_t number);
typedef int (*obj_handler_fn_t)(irq_t *obj, interrupt_frame_t *frame,
				void *private);

enum { kIrqNack = 0, kIrqAck = 1 };

typedef struct irq_vector {
	vector_t vector;
	int flags;
	size_t objectcnt;
	TAILQ_HEAD(irqlist, irq) objq;
} irq_vector_t;

typedef struct irq {
	irq_vector_t *vector;
	obj_handler_fn_t handler;
	void *private;

	TAILQ_ENTRY(irq) entry;
} irq_t;

void irq_init();

irq_t *irq_allocate_obj();
void irq_free_obj(irq_t *obj);
int port_register_isr(vector_t vec, irq_handler_fn_t isr);

int irq_initialize_obj_irql(irq_t *obj, irql_t irqlWanted, int flags);
