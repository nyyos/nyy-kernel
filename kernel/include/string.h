#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *dest, int value, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1_, const void *s2_, size_t n);
void *memmove(void *dest, const void *src, size_t n);

char *strcpy(char *dst, const char *src);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t len);

void bzero(void *s, size_t n);

#ifdef __cplusplus
}
#endif
