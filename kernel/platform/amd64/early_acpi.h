#pragma once

void acpi_early_set_rsdp(void *rsdp);
void* acpi_early_find(const char signature[4]);
