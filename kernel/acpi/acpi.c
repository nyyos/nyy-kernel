#include <stdint.h>
#include <assert.h>
#include <uacpi/uacpi.h>
#include <dkit/acpi.h>

void acpi_early_init(uintptr_t rsdp)
{
	struct uacpi_init_params params = {
		.rsdp = rsdp,
		.log_level = UACPI_LOG_INFO,
	};
	uacpi_status ret = uacpi_initialize(&params);
	assert(ret == UACPI_STATUS_OK);
}
