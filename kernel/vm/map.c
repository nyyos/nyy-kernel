#include <ndk/vm.h>
#include <ndk/ndk.h>
#include <assert.h>

vm_map_t map_kernel;

vm_map_t *vm_kmap()
{
	return &map_kernel;
}

vm_map_t *vm_map_new()
{
	assert(!"todo");
}
