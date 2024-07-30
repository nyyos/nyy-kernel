.text
.globl memset
  .type memset, @function
# rdi = dest ptr
# rsi = val
# rdx = size
memset:
  pushq %rbp
  pushq %rdi
  movq %rsp, %rbp

  cld

  movb %sil, %al
  movq %rdx, %rcx

  # we want to align it to 8
  movq $7, %r8
  and %r8, %rcx
  rep stosb

  sub %rcx, %rdx
  shr $3, %rdx
  movq %rdx, %rcx
  rep stosq

  popq %rdi
  popq %rbp
  ret
