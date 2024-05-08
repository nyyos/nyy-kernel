#pragma once

#include <nanoprintf.h>

void pac_putc(int ch, void *);

#define printf_wrapper(PUTC, ...) ({ npf_pprintf(PUTC, NULL, __VA_ARGS__); })

#define pac_printf(...) printf_wrapper(pac_putc, __VA_ARGS__)
