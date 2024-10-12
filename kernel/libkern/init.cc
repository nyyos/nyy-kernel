#include <cstddef>
#include <ndk/ndk.h>

extern "C" {
using InitPtr = void (*)();
extern InitPtr init_array_start[];
extern InitPtr init_array_end[];
}

extern "C" void libkern_init()
{
	if (*init_array_end == *init_array_start)
		return;

	size_t cnt = ((uintptr_t)init_array_end - (uintptr_t)init_array_start) /
		     sizeof(InitPtr);

	printk(DEBUG "running %ld global constructors\n", cnt);

	for (size_t i = 0; i < cnt; i++) {
		init_array_start[i]();
	}
}
