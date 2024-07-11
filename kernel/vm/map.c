#include "lib/vmem.h"
#include <ndk/vm.h>
#include <ndk/ndk.h>
#include <ndk/kmem.h>
#include <assert.h>

vm_map_t kernel_map;

vm_map_t *vm_kmap()
{
	return &kernel_map;
}

vm_map_t *vm_map_create()
{
	vm_map_t *map = kmalloc(sizeof(vm_map_t));
	vm_port_init_map(map);
	assert(!"todo");
}

void vm_map_destroy(vm_map_t *map)
{
	assert(!"todo");
}

void *vm_map_allocate(vm_map_t *map, size_t length)
{
	return vmem_alloc(map->arena, length, VM_INSTANTFIT);
}
