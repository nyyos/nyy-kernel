#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <ndk/addr.h>
#include <ndk/kmem.h>
#include <ndk/vm.h>
#include <ndk/ndk.h>
#include <DevKit/pci.h>
#include "asm.h"

#define MCFG_MAPPING_SIZE(buscount) (4096l * 8l * 32l * (buscount))

enum {
	PCI_IO_CONFIG_ADDRESS = 0xCF8,
	PCI_IO_CONFIG_DATA = 0xCFC,
};

static struct acpi_mcfg_allocation *mcfg_entries;
static size_t mcfg_entrycount;

static uint32_t pci_legacy_read(uint32_t segment, uint32_t bus, uint32_t slot,
				uint32_t function, uint32_t offset)
{
	assert(segment == 0);
	assert(bus < 256 && slot < 32 && function < 8 && offset < 256);
	assert(!(offset & 0b11));
	uint32_t address = 0x80000000 | (bus << 16) | (slot << 11) |
			   (function << 8) | (offset & ~(uint32_t)3);
	outl(PCI_IO_CONFIG_ADDRESS, address);
	return inl(PCI_IO_CONFIG_DATA);
}

static void pci_legacy_write(uint32_t segment, uint32_t bus, uint32_t slot,
			     uint32_t function, uint32_t offset, uint32_t value)
{
	assert(segment == 0);
	assert(bus < 256 && slot < 32 && function < 8 && offset < 256);
	assert(!(offset & 0b11));
	uint32_t address = 0x80000000 | (bus << 16) | (slot << 11) |
			   (function << 8) | (offset & ~(uint32_t)3);
	outl(PCI_IO_CONFIG_ADDRESS, address);
	outl(PCI_IO_CONFIG_DATA, value);
}

static struct acpi_mcfg_allocation *mcfg_bus(uint32_t segment, uint32_t bus)
{
	for (size_t i = 0; i < mcfg_entrycount; i++) {
		if (mcfg_entries[i].segment == segment &&
		    bus >= mcfg_entries[i].start_bus &&
		    bus <= mcfg_entries[i].end_bus) {
			return &mcfg_entries[i];
		}
	}
	return nullptr;
}

static uint32_t pci_mcfg_read(uint32_t segment, uint32_t bus, uint32_t slot,
			      uint32_t function, uint32_t offset)
{
	struct acpi_mcfg_allocation *entry = mcfg_bus(segment, bus);
	if (entry == nullptr) {
		printk("no entry for seg %d bus %d\n", segment, bus);
		return 0xFFFFFFFF;
	}

	volatile uint32_t *addr =
		(volatile uint32_t *)(entry->address +
					      ((bus - entry->start_bus) << 20 |
					       slot << 15 | function << 12) |
				      (offset & ~0x3));
	return *addr;
}

static void pci_mcfg_write(uint32_t segment, uint32_t bus, uint32_t slot,
			   uint32_t function, uint32_t offset, uint32_t value)
{
	struct acpi_mcfg_allocation *entry = mcfg_bus(segment, bus);

	volatile uint32_t *addr =
		(volatile uint32_t *)(entry->address +
					      ((bus - entry->start_bus) << 20 |
					       slot << 15 | function << 12) |
				      (offset & ~0x3));

	*addr = value;
}

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

		port_pci_read32 = pci_mcfg_read;
		port_pci_write32 = pci_mcfg_write;
	} else {
		printk(DEBUG "falling back to legacy PCI access\n");
		// fallback to legacy access
		port_pci_read32 = pci_legacy_read;
		port_pci_write32 = pci_legacy_write;
	}
}
