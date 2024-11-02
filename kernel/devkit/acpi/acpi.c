#include <stdint.h>
#include <assert.h>
#include <uacpi/uacpi.h>
#include <DevKit/acpi.h>
#include <nyyconf.h>


#if defined(CONFIG_LOG_LEVEL_ERROR)
#define LOG_LEVEL UACPI_LOG_ERROR
#elif defined(CONFIG_LOG_LEVEL_WARN)
#define LOG_LEVEL UACPI_LOG_WARN
#elif defined(CONFIG_LOG_LEVEL_INFO)
#define LOG_LEVEL UACPI_LOG_INFO
#elif defined(CONFIG_LOG_LEVEL_TRACE)
#define LOG_LEVEL UACPI_LOG_TRACE
#elif defined(CONFIG_LOG_LEVEL_DEBUG) || defined(CONFIG_LOG_LEVEL_SPAM)
#define LOG_LEVEL UACPI_LOG_DEBUG
#elif
#error "No log level"
#endif

void acpi_early_init(uintptr_t rsdp)
{
	struct uacpi_init_params params = {
		.rsdp = rsdp,
		.log_level = LOG_LEVEL,
	};
	uacpi_status ret = uacpi_initialize(&params);
	assert(ret == UACPI_STATUS_OK);
}
