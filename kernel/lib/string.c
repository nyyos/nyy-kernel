#include <stdint.h>
#include <stddef.h>
#include <string.h>

[[gnu::weak]] void *memset(void *p, int c, size_t n)
{
	char *str = (char *)p;
	for (size_t i = 0; i < n; i++) {
		str[i] = c;
	}
	return p;
}

void *memcpy(void *dest_, const void *src_, size_t n)
{
	uint8_t *dest = (uint8_t *)dest_;
	const uint8_t *src = (const uint8_t *)src_;
	for (size_t i = 0; i < n; i++) {
		dest[i] = src[i];
	}
	return dest_;
}

int memcmp(const void *s1_, const void *s2_, size_t n)
{
	const uint8_t *s1 = (const uint8_t *)(s1_);
	const uint8_t *s2 = (const uint8_t *)(s2_);

	for (size_t i = 0; i < n; i++) {
		if (s1[i] != s2[i])
			return s1[i] < s2[i] ? -1 : 1;
	}
	return 0;
}

void *memmove(void *dest, const void *src, size_t n)
{
	uint8_t *pdest = dest;
	const uint8_t *psrc = src;

	if (src > dest) {
		for (size_t i = 0; i < n; i++) {
			pdest[i] = psrc[i];
		}
	} else if (src < dest) {
		for (size_t i = n; i > 0; i--) {
			pdest[i - 1] = psrc[i - 1];
		}
	}

	return dest;
}

char *strncpy(char *dest, const char *src, size_t len)
{
	char *out = dest;

	while (*src && len--) {
		*dest = *src;
		dest++;
		src++;
	}

	return out;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	while (n && *s1 && (*s1 == *s2)) {
		++s1;
		++s2;
		--n;
	}
	if (n == 0) {
		return 0;
	} else {
		return (*(unsigned char *)s1 - *(unsigned char *)s2);
	}
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		++s1;
		++s2;
	}
	return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

char *strcpy(char *dest, const char *src)
{
	if (!dest)
		return NULL;

	char *ptr = dest;

	while (*src != '\0') {
		*dest = *src;
		dest++;
		src++;
	}

	*dest = '\0';

	return ptr;
}

size_t strlen(const char *str)
{
	size_t len = 0;
	for (len = 0; str[len]; len++)
		;

	return len;
}

void bzero(void *s, size_t n)
{
	char *_s = s;
	while (n--)
		_s[n] = 0x0;
}
