#include "DevKit/DevKit.h"
#include "nanoprintf.h"
#include "ndk/kmem.h"
#include <ndk/ndk.h>
#include <DevKit/pci.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

pci_read_fn *port_pci_read32 = nullptr;
pci_write_fn *port_pci_write32 = nullptr;

extern void port_pci_init();

uint8_t pci_read8(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		  uint32_t offset)
{
	return (port_pci_read32(seg, bus, slot, function, offset) >>
		((offset & 0b11) * 8)) &
	       0xff;
}

uint16_t pci_read16(uint32_t seg, uint32_t bus, uint32_t slot,
		    uint32_t function, uint32_t offset)
{
	return (port_pci_read32(seg, bus, slot, function, offset) >>
		((offset & 0b11) * 8)) &
	       0xffff;
}

uint32_t pci_read32(uint32_t seg, uint32_t bus, uint32_t slot,
		    uint32_t function, uint32_t offset)
{
	return port_pci_read32(seg, bus, slot, function, offset);
}

void pci_write8(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		uint32_t offset, uint8_t value)
{
	uint32_t old = port_pci_read32(seg, bus, slot, function, offset);
	int bitoffset = 8 * (offset & 0b11);
	old &= ~(0xff << bitoffset);
	old |= value << bitoffset;
	port_pci_write32(seg, bus, slot, function, offset, old);
}

void pci_write16(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		 uint32_t offset, uint16_t value)
{
	uint32_t old = port_pci_read32(seg, bus, slot, function, offset);
	int bitoffset = 8 * (offset & 0b11);
	old &= ~(0xffff << bitoffset);
	old |= value << bitoffset;
	port_pci_write32(seg, bus, slot, function, offset, old);
}

void pci_write32(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function,
		 uint32_t offset, uint32_t value)
{
	port_pci_write32(seg, bus, slot, function, offset, value);
}

/* utility functions */

uint8_t headerType(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function)
{
	uint32_t reg = pci_read32(seg, bus, slot, function, 0xC);
	return (reg & 0x00FF0000) >> 16;
}

uint16_t vendorId(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function)
{
	return pci_read16(seg, bus, slot, function, 0);
}

uint16_t deviceId(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function)
{
	return (pci_read32(seg, bus, slot, function, 0)) >> 16;
}

uint8_t classCode(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function)
{
	uint32_t reg3 = pci_read32(seg, bus, slot, function, 0x8);
	return reg3 >> 24;
}

uint8_t subClass(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function)
{
	uint32_t reg3 = pci_read32(seg, bus, slot, function, 0x8);
	return (reg3 & 0x00FF0000) >> 16;
}

uint8_t revisionId(uint32_t seg, uint32_t bus, uint32_t slot, uint32_t function)
{
	uint8_t reg3 = pci_read8(seg, bus, slot, function, 0x8);
	return reg3;
}

uint8_t secondaryBus(uint32_t seg, uint32_t bus, uint32_t slot,
		     uint32_t function)
{
	uint16_t reg6 = pci_read16(seg, bus, slot, function, 0x18);
	return reg6 >> 8;
}

/* main logic */

typedef struct pci_device {
	dev_node_t hdr;

	uint32_t seg, bus, slot, function;
} pci_device_t;

typedef struct pci_bus {
	pci_device_t node;

	uint32_t segId;
	uint32_t busId;
} pci_bus_t;

static int bus_cnt = 0;

pci_device_t *pci_create_device(uint32_t slot, uint32_t function)
{
	pci_device_t *dev = kmalloc(sizeof(pci_device_t));
	memset(dev, 0x0, sizeof(pci_device_t));
	char namebuf[16];
	npf_snprintf(namebuf, sizeof(namebuf), "PciDev-%d:%d", slot, function);
	dev_node_init(&dev->hdr, namebuf);
	return dev;
}

pci_bus_t *pci_create_bus(uint32_t seg, uint32_t bus)
{
	pci_bus_t *dev = kmalloc(sizeof(pci_bus_t));
	memset(dev, 0x0, sizeof(pci_bus_t));
	char namebuf[16];
	npf_snprintf(namebuf, sizeof(namebuf), "PciBus-%d:%d", seg, bus_cnt++);
	dev_node_init(&dev->node.hdr, namebuf);
	return dev;
}

static void check_bus(pci_bus_t *bus);

static void check_function(pci_bus_t *bus, uint32_t slot, uint32_t function)
{
	uint8_t classcode = classCode(bus->segId, bus->busId, slot, function);
	uint8_t subclass = subClass(bus->segId, bus->busId, slot, function);

	// host bridge
	if ((classcode == 0x6) && (subclass == 0x0))
		return;

	// pci-to-pci
	if ((classcode == 0x6) && (subclass == 0x4)) {
		uint8_t busId =
			secondaryBus(bus->segId, bus->busId, slot, function);
		pci_bus_t *dev = pci_create_bus(0, busId);
		dev_node_attach(&dev->node.hdr, bus->node.hdr.parent);
		dev->segId = 0;
		dev->busId = busId;
		dev->node.seg = bus->segId;
		dev->node.bus = bus->busId;
		dev->node.slot = slot;
		dev->node.function = function;
		check_bus(dev);
	}

	pci_device_t *dev = pci_create_device(slot, function);
	dev_node_attach(&dev->hdr, &bus->node.hdr);
}

static void check_slot(pci_bus_t *bus, uint32_t slot)
{
	uint16_t vendor;
	uint8_t function = 0;

	vendor = vendorId(bus->segId, bus->busId, slot, function);
	if (vendor == 0xFFFF) {
		return;
	}
	check_function(bus, slot, function);
	if ((headerType(bus->segId, bus->busId, slot, function) & 0x80) != 0) {
		for (function = 1; function < 8; function++) {
			if (vendorId(bus->segId, bus->busId, slot, function) !=
			    0xFFFF) {
				check_function(bus, slot, function);
			}
		}
	}
}

static void check_bus(pci_bus_t *bus)
{
	for (uint8_t slot = 0; slot < 32; slot++) {
		check_slot(bus, slot);
	}
}

void pci_init(dev_node_t *proot)
{
	// setup pci_read/write
	port_pci_init();

	uint8_t hdr = headerType(0, 0, 0, 0);
	if ((hdr & 0x80) == 0) {
		pci_bus_t *bus = pci_create_bus(0, 0);
		dev_node_attach(&bus->node.hdr, proot);
		bus->segId = 0;
		bus->busId = 0;
		bus->node.seg = 0;
		bus->node.bus = 0;
		bus->node.slot = 0;
		bus->node.function = 0;
		check_bus(bus);
	} else {
		for (uint8_t function = 0; function < 8; function++) {
			if (vendorId(0, 0, 0, function) == 0xFFFF)
				break;
			pci_bus_t *bus = pci_create_bus(0, function);
			dev_node_attach(&bus->node.hdr, proot);
			bus->segId = 0;
			bus->busId = function;
			bus->node.seg = 0;
			bus->node.bus = 0;
			bus->node.slot = 0;
			bus->node.function = function;
			check_bus(bus);
		}
	}
}
