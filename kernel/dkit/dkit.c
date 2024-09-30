#include <dkit/dkit.h>

[[gnu::weak]] void port_pci_init() {}

void dkit_init()
{
	port_pci_init();
}
