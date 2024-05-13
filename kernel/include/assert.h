#pragma once

#include <nyyconf.h>

__attribute__((__noreturn__)) void __assert_fail(const char *assertion,
						 const char *file,
						 unsigned int line,
						 const char *function);

#ifdef CONFIG_DISABLE_ASSERTS

#undef assert
#define assert(...) ((void)0)

#else

#undef assert
#define assert(expr)      \
	((void)((expr) || \
		(__assert_fail(#expr, __FILE__, __LINE__, __func__), 0)))

#endif
