kernel_sources += files('main.c')

subdir('lib')
subdir('vm')
subdir('irq')
subdir('boot')
subdir('sched')
subdir('dkit')

# XXX: make this depend on config
subdir('acpi')

if (use_limine)
  subdir('platform/limine-generic')
endif

if (arch == 'amd64')
  subdir('platform/amd64')
elif (arch == 'riscv64')
  subdir('platform/riscv64')
elif (arch == 'linux')
  subdir('platform/linux')
endif
