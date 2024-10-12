#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void acpi_early_init(uintptr_t rsdp);

#ifdef __cplusplus
}
#endif
