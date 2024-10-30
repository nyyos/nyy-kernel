#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <DevKit/DevKit.h>

#ifdef NYY_PRIVATE
typedef uint32_t(pci_read_fn)(uint32_t seg, uint32_t bus, uint32_t slot,
			      uint32_t function, uint32_t offset);

typedef void(pci_write_fn)(uint32_t seg, uint32_t bus, uint32_t slot,
			   uint32_t function, uint32_t offset, uint32_t value);

extern pci_read_fn *port_pci_read32;
extern pci_write_fn *port_pci_write32;
#endif

void pci_init(dev_node_t *proot);

uint8_t pci_read8(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		  uint32_t offset);
uint16_t pci_read16(uint32_t seg, uint32_t bus, uint32_t slot,
		    uint32_t function, uint32_t offset);
uint32_t pci_read32(uint32_t seg, uint32_t bus, uint32_t slot,
		    uint32_t function, uint32_t offset);

void pci_write8(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		uint32_t offset, uint8_t value);
void pci_write16(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		 uint32_t offset, uint16_t value);
void pci_write32(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		 uint32_t offset, uint32_t value);

#ifdef __cplusplus
}
#endif
