.data
trampoline_exit:
  .asciz "TRAMPOLINE RETURNED\n"


  .text
.global thread_trampoline
.extern irql_lower 
.extern panic

# registers on function entry:
# r12 startfn
# r13 context1
# r14 context2
thread_trampoline:
  xor %rbp, %rbp
  
  push %rbp
  mov %rsp, %rbp

  # we start on DPC level, so lower that
  mov $0, %rdi
  call irql_lower 

  mov %r13, %rdi
  mov %r14, %rsi
  call *%r12

  lea trampoline_exit(%rip), %rdi
  call panic
