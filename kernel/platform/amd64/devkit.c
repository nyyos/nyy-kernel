
#include "DevKit/DevKit.h"
#include <DevKit/pci.h>

extern void port_devkit_init()
{
	dev_node_t *dtroot = gDeviceTree.plane_root;
	dev_node_t *proot = dev_node_create("AMD64Platform");
	dev_node_attach(proot, dtroot);

	pci_init(proot);
}
