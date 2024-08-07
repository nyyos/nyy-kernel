subdir('lib')
subdir('vm')
subdir('irq')

# XXX: make this depend on arch?
subdir('acpi')

if (use_limine)
  subdir('platform/limine-generic')
endif

if (arch == 'amd64')
  subdir('platform/amd64')
elif (arch == 'riscv64')
  subdir('platform/riscv64-virt')
elif (arch == 'linux')
  subdir('platform/linux')
endif
