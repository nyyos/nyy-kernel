#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <nyyconf.h>

#include <nanoprintf.h>

#include <ndk/irql.h>
#include <ndk/port.h>

#define SPAM "\1"
#define DEBUG "\2"
#define TRACE "\3"
#define INFO "\4"
#define WARN "\5"
#define ERR "\6"
#define PANIC "\7"

__attribute__((format(printf, 1, 2))) void printk(const char *fmt, ...);
__attribute__((format(printf, 1, 2))) void printk_locked(const char *fmt, ...);
void _printk_consoles_write(const char *buf, size_t size);
void printk_init();

[[gnu::noreturn]] void panic(const char *msg);
[[gnu::noreturn]] void panic_withstack(const char *msg, uintptr_t stack);

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

[[nodiscard]] static inline int spinlock_try_lock_intr(spinlock_t *spinlock)
{
	int old = port_set_ints(0);
	bool res = spinlock_try_lock(spinlock);
	if (res)
		return old;
	port_set_ints(old);
	return -1;
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

[[nodiscard]] static inline int spinlock_acquire_intr(spinlock_t *spinlock)
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

#ifdef __cplusplus
}
#endif
