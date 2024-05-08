#include <ndk/ndk.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

void *memset(void *p, int c, size_t n)
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

void spinlock_acquire(spinlock_t *spinlock)
{
	while (atomic_flag_test_and_set(&spinlock->flag)) {
#ifdef __x86_64__
		asm volatile("pause");
#endif
	}
}

void spinlock_release(spinlock_t *spinlock)
{
	atomic_flag_clear(&spinlock->flag);
}
