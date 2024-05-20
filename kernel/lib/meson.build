kernel_sources += files('log.c', 'std.c', 'vmem.c','string.c')

if conf.get('CONFIG_UBSAN', 'n') == 'y'
  kernel_sources += files('tinyubsan.c')
endif
