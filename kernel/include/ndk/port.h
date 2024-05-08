#pragma once

#ifdef AMD64
#include "amd64.h"
#elif defined(__riscv)
#if __riscv_xlen == 64
#include "riscv64.h"
#else
#error "riscv32 not supported"
#endif
#else
#error "port unsupported"
#endif
