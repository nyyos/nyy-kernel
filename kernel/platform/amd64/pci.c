#include "ndk/kmem.h"
#include "ndk/vm.h"
#include <stdint.h>
#include <string.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <assert.h>
#include <ndk/addr.h>
#include <ndk/ndk.h>
#include "asm.h"

enum {
	PCI_IO_CONFIG_ADDRESS = 0xCF8,
	PCI_IO_CONFIG_DATA = 0xCFC,
};

uint32_t pci_legacy_read_32(uint32_t bus, uint32_t slot, uint32_t function,
			    uint32_t offset)
{
	assert(bus < 256 && slot < 32 && function < 8 && offset < 256);
	assert(!(offset & 0b11));
	uint32_t address = 0x80000000 | (bus << 16) | (slot << 11) |
			   (function << 8) | (offset & ~0x3);
	outl(PCI_IO_CONFIG_DATA, address);
	return inl(PCI_IO_CONFIG_ADDRESS);
}

void pci_legacy_write_32(uint32_t bus, uint32_t slot, uint32_t function,
			 uint32_t offset, uint32_t value)
{
	assert(bus < 256 && slot < 32 && function < 8 && offset < 256);
	assert(!(offset & 0b11));
	uint32_t address = 0x80000000 | (bus << 16) | (slot << 11) |
			   (function << 8) | (offset & ~0x3);
	outl(PCI_IO_CONFIG_DATA, address);
	outl(PCI_IO_CONFIG_DATA, value);
}

static struct acpi_mcfg_allocation *mcfg_entries;
static size_t mcfg_entrycount;

#define MCFG_MAPPING_SIZE(buscount) (4096l * 8l * 32l * (buscount))

void port_pci_init()
{
	uacpi_table tbl;
	int ret = uacpi_table_find_by_signature("MCFG", &tbl);
	if (ret == UACPI_STATUS_OK) {
		printk(DEBUG "MCFG found\n");
		struct acpi_mcfg *mcfg = tbl.ptr;

		mcfg_entrycount =
			(mcfg->hdr.length - sizeof(mcfg->hdr) - (mcfg->rsvd)) /
			sizeof(struct acpi_mcfg_allocation);

		mcfg_entries = kmalloc(sizeof(struct acpi_mcfg_allocation) *
				       mcfg_entrycount);

		memcpy(mcfg_entries, mcfg->entries,
		       mcfg_entrycount * sizeof(struct acpi_mcfg_allocation));

		for (int i = 0; i < mcfg_entrycount; i++) {
			struct acpi_mcfg_allocation *entry = &mcfg_entries[i];
			uintptr_t poff = entry->address % PAGE_SIZE;
			size_t mapsz = MCFG_MAPPING_SIZE(entry->end_bus -
							 entry->start_bus);

			uintptr_t phys = PAGE_ALIGN_DOWN(entry->address);
			uintptr_t virt =
				(uintptr_t)vm_map_allocate(vm_kmap(), mapsz);

			for (size_t i = 0;
			     i < PAGE_ALIGN_UP(mapsz + poff) / PAGE_SIZE; i++) {
				vm_port_map(vm_kmap(),
					    PADDR(phys + i * PAGE_SIZE),
					    VADDR(virt + i * PAGE_SIZE),
					    kVmUncached, kVmRead | kVmWrite);
			}

			entry->address = virt + poff;

			printk(DEBUG
			       "mcfg entry %d: addr:%#lx segment:%d startbus:%d endbus:%d\n",
			       i, entry->address, entry->segment,
			       entry->start_bus, entry->end_bus);
		}
	} else {
		// fallback to legacy access
	}
}
