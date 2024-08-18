.text
.global _start
.extern ultra_entry
.extern limine_entry
_start:
  cmp $0x554c5442, %rsi
  je ultra_entry
  jmp limine_entry
