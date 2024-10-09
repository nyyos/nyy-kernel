#include <ndk/ndk.h>

extern "C" [[noreturn]] void abort() noexcept
{
	panic("abort");
}
