#pragma once

#include <nanoprintf.h>
#include <stdatomic.h>

#include "ndk/irql.h"

typedef struct spinlock {
	atomic_flag flag;
} spinlock_t;

#define SPINLOCK_INIT(spinlock) atomic_flag_clear(&(spinlock)->flag);
#define SPINLOCK_WAIT(spinlock) spinlock_acquire(spinlock, HIGH_LEVEL);

irql_t spinlock_acquire(spinlock_t *spinlock, irql_t at);
void spinlock_release(spinlock_t *spinlock, irql_t before);

extern spinlock_t g_pac_lock;
void pac_putc(int ch, void *);

#define printf_wrapper(PUTC, ...)                                  \
	({                                                         \
		irql_t _printf_old_irql =                          \
			spinlock_acquire(&g_pac_lock, HIGH_LEVEL); \
		npf_pprintf(PUTC, NULL, __VA_ARGS__);              \
		spinlock_release(&g_pac_lock, _printf_old_irql);   \
	})

#define pac_printf(...) printf_wrapper(pac_putc, __VA_ARGS__)
