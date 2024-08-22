.text
.globl memset
  .type memset, @function
# rdi = dest ptr
# rsi = val
# rdx = size
memset:
  cld
  mov %rdi, %rax
  movb %sil, %al
  movq %rdx, %rcx
  rep stosb
  ret

.globl memcpy
  .type memcpy, @function
# rdi = dest ptr
# rsi = src ptr
# rdx = count
memcpy:
  cld
  movq %rdx, %rcx
  rep movsb
  ret
