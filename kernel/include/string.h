#pragma once

#include <stddef.h>

void *memset(void *dest, int value, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1_, const void *s2_, size_t n);
