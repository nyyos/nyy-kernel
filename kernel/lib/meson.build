kernel_sources += files(
  'log.c', 'std.c', 'vmem.c',
  'string.c', 'panic.c', 'symbols.c',
  'slab.c', 'time.c', 'mutex.c'
)

if conf.get('CONFIG_UBSAN', 'n') == 'y'
  kernel_sources += files('tinyubsan.c')
endif

if conf.get('CONFIG_STACKGUARD', 'n') == 'y'
  kernel_sources += files('stackguard.c')
endif
