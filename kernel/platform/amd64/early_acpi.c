#include <string.h>
#include <uacpi/acpi.h>
#include <assert.h>

#include <ndk/vm.h>
#include <ndk/ndk.h>

#include "early_acpi.h"

static struct acpi_rsdp *g_rsdp;

void *acpi_early_find(const char signature[4])
{
	if (g_rsdp->revision < 2) {
		struct acpi_rsdt *rsdt =
			(struct acpi_rsdt *)N2V(g_rsdp->rsdt_addr).addr;
		size_t entries = (rsdt->hdr.length - sizeof(rsdt->hdr)) / 4;
		for (size_t i = 0; i < entries; i++) {
			struct acpi_sdt_hdr *hdr =
				(struct acpi_sdt_hdr *)N2V(rsdt->entries[i])
					.addr;
			if (strncmp(hdr->signature, signature, 4) == 0)
				return hdr;
		}
	} else {
		assert(g_rsdp->xsdt_addr != 0);
		struct acpi_xsdt *xsdt =
			(struct acpi_xsdt *)N2V(g_rsdp->xsdt_addr).addr;
		size_t entries = (xsdt->hdr.length - sizeof(xsdt->hdr)) / 8;
		for (size_t i = 0; i < entries; i++) {
			struct acpi_sdt_hdr *hdr =
				(struct acpi_sdt_hdr *)N2V(xsdt->entries[i])
					.addr;
			if (strncmp(hdr->signature, signature, 4) == 0)
				return hdr;
		}
	}
	return nullptr;
}
