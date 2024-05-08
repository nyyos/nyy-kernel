#pragma once

#include <nanoprintf.h>
#include <stdatomic.h>

typedef struct spinlock {
	atomic_flag flag;
} spinlock_t;

#define SPINLOCK_INIT(spinlock) atomic_flag_clear(&(spinlock)->flag)

void spinlock_acquire(spinlock_t *spinlock);
void spinlock_release(spinlock_t *spinlock);

void pac_putc(int ch, void *);

#define printf_wrapper(PUTC, ...) ({ npf_pprintf(PUTC, NULL, __VA_ARGS__); })

#define pac_printf(...) printf_wrapper(pac_putc, __VA_ARGS__)
