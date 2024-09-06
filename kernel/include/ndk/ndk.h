#pragma once

#include <stdbool.h>

#include <nyyconf.h>

#include <nanoprintf.h>

#include <ndk/irql.h>
#include <ndk/port.h>

typedef struct spinlock {
	volatile uint8_t flag;
} spinlock_t;

#define SPINLOCK_INIT(spinlock) ((spinlock)->flag = 0);
// clang-format off
#define SPINLOCK_INITIALIZER() {0}
// clang-format on

static inline void spinlock_release(spinlock_t *spinlock)
{
	__atomic_store_n(&spinlock->flag, 0, __ATOMIC_RELEASE);
}

static inline void spinlock_acquire(spinlock_t *spinlock)
{
	uint8_t unlocked = 0;
	while (!__atomic_compare_exchange_n(&spinlock->flag, &unlocked, 1, 0,
					    __ATOMIC_ACQUIRE,
					    __ATOMIC_RELAXED)) {
		unlocked = 0;
		port_spin_hint();
	}
}

static inline bool spinlock_held(spinlock_t *spinlock)
{
	return __atomic_load_n(&spinlock->flag, __ATOMIC_RELAXED);
}

static inline bool spinlock_try_lock(spinlock_t *spinlock)
{
	uint8_t unlocked = 0;
	return __atomic_compare_exchange_n(&spinlock->flag, &unlocked, 1, 0,
					   __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}
static inline void spinlock_release_lower(spinlock_t *spinlock, irql_t old)
{
	spinlock_release(spinlock);
	irql_lower(old);
}

[[nodiscard]] static inline irql_t spinlock_acquire_raise(spinlock_t *spinlock)
{
	irql_t old = irql_raise(IRQL_DISPATCH);
	spinlock_acquire(spinlock);
	return old;
}

static inline int spinlock_acquire_intr(spinlock_t *spinlock)
{
	int old = port_set_ints(0);
	spinlock_acquire(spinlock);
	return old;
}

static inline void spinlock_release_intr(spinlock_t *spinlock, int old)
{
	spinlock_release(spinlock);
	port_set_ints(old);
}

#define INFO "\1"
#define WARN "\2"
#define ERR "\3"
#define DEBUG "\4"
#define PANIC "\5"

__attribute__((format(printf, 1, 2))) void printk(const char *fmt, ...);
__attribute__((format(printf, 1, 2))) void printk_locked(const char *fmt, ...);
void _printk_consoles_write(const char *buf, size_t size);
void _printk_init();

[[gnu::noreturn]] void panic(const char *msg);
