.text
.global _start
.extern ultra_entry
.extern limine_entry

#include <nyyconf.h>


_start:
#ifdef CONFIG_STACKGUARD
.extern __stack_chk_guard
  push %rdi
  push %rsi

rdrand:
  // check if rdrand is available
  mov $1, %eax
  xor %ecx, %ecx
  cpuid
  bt $30, %ecx
  jnc rdseed
  rdrand %rax
  jmp move

rdseed:
  // check if rdseed is available
  mov $7, %eax
  xor %ecx, %ecx
  cpuid
  bt $18, %ebx
  jnc exit

move:
  // move random value from rax to stack guard
  mov %rax, __stack_chk_guard(%rip)

exit:
  // if we jump to exit, we didn't have a valid random instruction 
  pop %rsi
  pop %rdi
#endif

  cmp $0x554c5442, %rsi
  je ultra_entry
  jmp limine_entry
