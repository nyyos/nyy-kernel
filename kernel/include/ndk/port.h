#pragma once

#if defined(AMD64)
#include "ports/amd64.h"
#elif defined(__riscv)
#if __riscv_xlen == 64
#include "ports/riscv64.h"
#else
#error "riscv32 not supported"
#endif
#elif defined(NYYLINUX)
#include "ports/linux.h"
#else
#error "port unsupported"
#endif

#ifndef ARCH_HAS_SPIN_HINT
#define port_spin_hint() (void)0
#endif
