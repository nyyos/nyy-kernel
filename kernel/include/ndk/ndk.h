#pragma once

#include <nanoprintf.h>

#include "ndk/irql.h"
#include <ndk/port.h>

typedef struct spinlock {
	volatile int flag;
} spinlock_t;

#define SPINLOCK_WAIT(spinlock) spinlock_acquire(spinlock, HIGH_LEVEL);
#define SPINLOCK_INIT(spinlock) ((spinlock)->flag = 0);
// clang-format off
#define SPINLOCK_INITIALIZER() {0}
// clang-format on

static inline void spinlock_release_no_irql(spinlock_t *spinlock)
{
	__atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELEASE);
}

static inline void spinlock_acquire_no_irql(spinlock_t *spinlock)
{
	int unlocked = 0;
	while (!__atomic_compare_exchange_n(&spinlock->flag, &unlocked, 1, 0,
					    __ATOMIC_ACQUIRE,
					    __ATOMIC_RELAXED)) {
		unlocked = 0;
		port_spin_hint();
	}
}

static inline bool spinlock_try_lock_no_irql(spinlock_t *spinlock)
{
	int unlocked = 0;
	return __atomic_compare_exchange_n(&spinlock->flag, &unlocked, 1, 0,
					   __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

static inline bool spinlock_try_lock(spinlock_t *spinlock, irql_t at,
				     irql_t *old)
{
	if (spinlock_try_lock_no_irql(spinlock)) {
		*old = irql_raise(at);
		return true;
	}
	return false;
}

static inline void spinlock_release(spinlock_t *spinlock, irql_t to)
{
	spinlock_release_no_irql(spinlock);
	irql_lower(to);
}

static inline irql_t spinlock_acquire(spinlock_t *spinlock, irql_t at)
{
	irql_t old = irql_raise(at);
	spinlock_acquire_no_irql(spinlock);
	return old;
}

#define INFO "\1"
#define WARN "\2"
#define ERR "\3"
#define DEBUG "\4"
#define PANIC "\5"

void printk(const char *fmt, ...);
void printk_locked(const char *fmt, ...);
void _printk_consoles_write(const char *buf, size_t size);
void _printk_init();

[[gnu::noreturn]] void panic(const char *msg);
