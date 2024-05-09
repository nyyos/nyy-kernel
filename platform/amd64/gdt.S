.global cpu_gdt_load_

.text

// rdi: gdtr
// rsi: cs
// rdx: ss
cpu_gdt_load_:
lgdt (%rdi)
push %rsi
lea _reload_ds(%rip), %rax
push %rax
lretq
_reload_ds:
mov %dx, %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %ss
xor %ax, %ax
mov %ax, %gs
mov %ax, %fs
ret
