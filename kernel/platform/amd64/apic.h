#pragma once

#include <stdint.h>

void apic_init();
void apic_enable();
void apic_eoi();
uint8_t apic_id();
uint8_t apic_version();
void apic_arm(uint64_t deadline);
