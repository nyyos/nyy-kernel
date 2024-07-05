.text

.extern handle_fault
.extern handle_irq

.set .L_typeFaultNoCode, 1
.set .L_typeFaultCode, 2

.macro FAULT_STUB type, number=0
irqStub\number:
.if \type == .L_typeFaultNoCode
push $0
.endif
# do swapgs
push %rbp
push %r15
push %r14
push %r13
push %r12
push %r11
push %r10
push %r9
push %r8
push %rdi
push %rsi
push %rdx
push %rcx
push %rbx
push %rax

# do fault
mov %rsp, %rdi
mov $\number, %rsi
xor %rbp, %rbp
call handle_fault

pop %rax
pop %rbx
pop %rcx
pop %rdx
pop %rsi
pop %rdi
pop %r8
pop %r9
pop %r10
pop %r11
pop %r12
pop %r13
pop %r14
pop %r15
pop %rbp

add $8, %rsp
iretq
.endm

.macro IRQ_STUB number=0
irqStub\number:
push $0
# do swapgs
push %rbp
push %r15
push %r14
push %r13
push %r12
push %r11
push %r10
push %r9
push %r8
push %rdi
push %rsi
push %rdx
push %rcx
push %rbx
push %rax

# do irq
mov %rsp, %rdi
mov $\number, %rsi
xor %rbp, %rbp
call handle_irq

pop %rax
pop %rbx
pop %rcx
pop %rdx
pop %rsi
pop %rdi
pop %r8
pop %r9
pop %r10
pop %r11
pop %r12
pop %r13
pop %r14
pop %r15
pop %rbp

add $8, %rsp
iretq
.endm

.macro SPECIAL_STUB function, number
.extern \function 
irqStub\number:
push $0
# do swapgs
push %rbp
push %r15
push %r14
push %r13
push %r12
push %r11
push %r10
push %r9
push %r8
push %rdi
push %rsi
push %rdx
push %rcx
push %rbx
push %rax

# do fault
mov %rsp, %rdi
xor %rbp, %rbp
call \function

pop %rax
pop %rbx
pop %rcx
pop %rdx
pop %rsi
pop %rdi
pop %r8
pop %r9
pop %r10
pop %r11
pop %r12
pop %r13
pop %r14
pop %r15
pop %rbp

add $8, %rsp
iretq
.endm

FAULT_STUB .L_typeFaultNoCode, 0
FAULT_STUB .L_typeFaultNoCode, 1
FAULT_STUB .L_typeFaultNoCode, 2
FAULT_STUB .L_typeFaultNoCode, 3
FAULT_STUB .L_typeFaultNoCode, 4
FAULT_STUB .L_typeFaultNoCode, 5
FAULT_STUB .L_typeFaultNoCode, 6
FAULT_STUB .L_typeFaultNoCode, 7
FAULT_STUB .L_typeFaultCode, 8
FAULT_STUB .L_typeFaultCode, 10 
FAULT_STUB .L_typeFaultCode, 11
FAULT_STUB .L_typeFaultCode, 12
FAULT_STUB .L_typeFaultCode, 13
FAULT_STUB .L_typeFaultCode, 14
FAULT_STUB .L_typeFaultNoCode, 16
FAULT_STUB .L_typeFaultCode, 17
FAULT_STUB .L_typeFaultNoCode, 18
FAULT_STUB .L_typeFaultNoCode, 19
# some hypervisor faults

.altmacro
.set i,32
.rept 224
  IRQ_STUB %i
  .set i,i+1
.endr

.macro DEC_INT num
.quad irqStub\num
.endm

.data
.global itable
.align 8
itable:
DEC_INT 0
DEC_INT 1
DEC_INT 2
DEC_INT 3
DEC_INT 4
DEC_INT 5
DEC_INT 6
DEC_INT 7
DEC_INT 8
.quad 0x00000000
DEC_INT 10 
DEC_INT 11
DEC_INT 12
DEC_INT 13
DEC_INT 14
.quad 0x00000000
DEC_INT 16
DEC_INT 17
DEC_INT 18
DEC_INT 19

.rept 12
.quad 0x00000000
.endr

.set i,32
.rept 224
DEC_INT %i
.set i,i+1
.endr
