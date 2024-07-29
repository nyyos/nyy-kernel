#pragma once

#include <nanoprintf.h>

#include "ndk/irql.h"

typedef struct spinlock {
	volatile int flag;
} spinlock_t;

#define SPINLOCK_WAIT(spinlock) spinlock_acquire(spinlock, HIGH_LEVEL);
#define SPINLOCK_INIT(spinlock) ((spinlock)->flag = 0);
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

void printk(const char *fmt, ...);
void printk_locked(const char *fmt, ...);
void _printk_consoles_write(const char *buf, size_t size);
void _printk_init();

[[gnu::noreturn]] void panic(const char* msg);
