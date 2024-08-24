#pragma once

#include <stdint.h>
#include <ndk/irq.h>

void apic_init();
void apic_enable();
void apic_eoi();
uint8_t apic_id();
uint8_t apic_version();
void apic_arm(uint64_t deadline);

void apic_send_ipi(uint32_t lapic_id, vector_t vector);
