#pragma once

#if defined(AMD64)
#include "ports/amd64.h" // IWYU pragma: export
#elif defined(__riscv)
#if __riscv_xlen == 64
#include "ports/riscv64.h" // IWYU pragma: export
#else
#error "riscv32 not supported"
#endif
#elif defined(NYYLINUX)
#include "ports/linux.h" // IWYU pragma: export
#else
#error "port unsupported"
#endif

#ifndef ARCH_HAS_SPIN_HINT
#define port_spin_hint() (void)0
#endif
