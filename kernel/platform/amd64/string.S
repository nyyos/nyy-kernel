.text
.globl memset
  .type memset, @function
# rdi = dest ptr
# rsi = val
# rdx = size
memset:
  pushq %rbp
  movq %rsp, %rbp
  mov %rdi, %rax

  cld

  movb %sil, %al
  movq %rdx, %rcx
  rep stosb

  popq %rbp
  ret
