#include <DevKit/DevKit.h>

extern void port_devkit_init();

extern char builtin_module_start[];
extern char builtin_module_end[];

dev_node_t gDtRootNode;

dev_plane_t gDeviceTree = {
	.plane_name = "DeviceTree",
	.plane_root = &gDtRootNode,
};

static void load_modules()
{
}

void devkit_init()
{
	load_modules();

	dev_node_init(&gDtRootNode, "Root");
	port_devkit_init();

	dev_print_tree(gDeviceTree.plane_root);
}
