#pragma once

#include <nanoprintf.h>
#include <stdatomic.h>

#include "ndk/irql.h"

typedef struct spinlock {
	atomic_flag flag;
} spinlock_t;

#define SPINLOCK_WAIT(spinlock) spinlock_acquire(spinlock, HIGH_LEVEL);
#define SPINLOCK_INIT(spinlock) atomic_flag_clear(&(spinlock)->flag);
// clang-format off
#define SPINLOCK_INITIALIZER() {0}
// clang-format on
irql_t spinlock_acquire(spinlock_t *spinlock, irql_t at);
void spinlock_release(spinlock_t *spinlock, irql_t before);

#define INFO "\1"
#define WARN "\2"
#define ERR "\3"
#define DEBUG "\4"
#define PANIC "\5"

extern spinlock_t printk_lock;
#define printk(...)                                                  \
	do {                                                              \
		irql_t irql = spinlock_acquire(&printk_lock, HIGH_LEVEL); \
		printk_locked(__VA_ARGS__);                         \
		spinlock_release(&printk_lock, irql);                     \
	} while (0)

void printk_locked(const char *fmt, ...);
void _printk_consoles_write(const char* buf, size_t size);

[[gnu::noreturn]] void panic();
