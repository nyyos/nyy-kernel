static void hcf() {
  for (;;) {
    asm volatile("cli;hlt");
  }
}

void _start(void) { hcf(); }
