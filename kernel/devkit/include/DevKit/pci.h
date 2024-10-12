#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint32_t(pci_read_fn)(uint32_t seg, uint32_t bus, uint32_t slot,
			      uint32_t function, uint32_t offset);

typedef void(pci_write_fn)(uint32_t seg, uint32_t bus, uint32_t slot,
			   uint32_t function, uint32_t offset, uint32_t value);

extern pci_read_fn *pci_read32;
extern pci_write_fn *pci_write32;

#ifdef __cplusplus
}
#endif
