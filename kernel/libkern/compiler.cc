#include <ndk/ndk.h>

extern "C" [[noreturn]] void abort() noexcept
{
	panic("abort");
}

extern "C" int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle)
{
	return 0;
}

extern "C" void __cxa_pure_virtual()
{
	panic("__cxa_pure_virtual");
}
