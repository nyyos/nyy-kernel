#include <cstddef>
#include <ndk/ndk.h>
#include <libkern/OSMetaClass.h>
#include <cassert>

extern "C" {
using InitPtr = void (*)();
extern InitPtr init_array_start[];
extern InitPtr init_array_end[];
}

extern "C" void libkern_init()
{
	size_t cnt;
	OSMetaClass::initialize();
	auto handle = OSMetaClass::preModLoad("kernel");

	if (*init_array_end == *init_array_start)
		goto finish;

	cnt = ((uintptr_t)init_array_end - (uintptr_t)init_array_start) /
	      sizeof(InitPtr);

	printk(DEBUG "running %ld global constructors\n", cnt);

	for (size_t i = 0; i < cnt; i++) {
		init_array_start[i]();
	}

finish:
	assert(OSMetaClass::checkModLoad(handle));
	OSMetaClass::postModLoad(handle);
}
