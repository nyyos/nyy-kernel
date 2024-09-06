.data
trampoline_exit:
  .asciz "TRAMPOLINE RETURNED\n"


.text
.extern irql_lower 
.extern panic

# rdi new thread 
# rsi old thread 
# only save rbx, rsp, rbp, r12-15
.global asm_switch_context
.global asm_jumpstart
asm_switch_context:
  mov %rbx, 8(%rsi)
  mov %r12, 80(%rsi)
  mov %r13, 88(%rsi)
  mov %r14, 96(%rsi)
  mov %r15, 104(%rsi)
  mov %rbp, 112(%rsi)
  mov %rsp, 144(%rsi) 
  # release lock
  movb $0, 160(%rsi)

asm_jumpstart:
  mov 8(%rdi), %rbx
  mov 80(%rdi), %r12
  mov 88(%rdi), %r13
  mov 96(%rdi), %r14
  mov 104(%rdi), %r15
  mov 112(%rdi), %rbp
  mov 144(%rdi), %rsp

  ret

# registers on function entry:
# r12 startfn
# r13 context1
# r14 context2
.global thread_trampoline
thread_trampoline:
  xor %rbp, %rbp
 
  push %rbp
  mov %rsp, %rbp

  # we start on DPC level, so lower that
  mov $0, %rdi
  call irql_lower 

  sti

  mov %r13, %rdi
  mov %r14, %rsi
  call *%r12

  lea trampoline_exit(%rip), %rdi
  call panic
