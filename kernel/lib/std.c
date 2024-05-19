#include <ndk/ndk.h>
#include <ndk/cpudata.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

spinlock_t g_pac_lock;

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

irql_t spinlock_acquire(spinlock_t *spinlock, irql_t at)
{
	while (atomic_flag_test_and_set(&spinlock->flag)) {
#ifdef __x86_64__
		asm volatile("pause");
#endif
	}
	return irql_raise(at);
}

void spinlock_release(spinlock_t *spinlock, irql_t to)
{
	atomic_flag_clear(&spinlock->flag);
	irql_lower(to);
}

void cpudata_setup(cpudata_t *cpudata)
{
	memset(cpudata, 0x0, sizeof(cpudata_t));
	cpudata_port_setup(&cpudata->port_data);
}

void __assert_fail(const char *assertion, const char *file, unsigned int line,
		   const char *function)
{
	npf_pprintf(
		pac_putc, nullptr,
		"Assertion failure at %s:%d in function %s\nAssertion: %s\n",
		file, line, function, assertion);
	hcf();
}
