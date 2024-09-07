ultra_protocol = dependency('ultra_protocol')

nyy = executable('nyy', 
  'entry.c', 'gdt.c', 'int.c',
  'gdt.S', 'stubs.S', 'vm.c',
  'string.S', 'start.S', 'ultra.c', 
  'pic.c', 'hpet.c',
  'apic.c', 'sched.S', 'sched.c',
  kernel_sources,
  c_args: [ freestanding_c_args, '-std=gnu2x'],
  include_directories: [freestanding_include_dirs],
  link_args: [
    '-nostdlib', 
    '-static',
    '-Wl,-zmax-page-size=0x1000',
    '-Wl,-T'+meson.current_source_dir()+'/linker.ld'
  ],
  dependencies: [
    kernel_deps,
    ultra_protocol
  ]
)

custom_target('generate symbols',
          depends : nyy,
          input : nyy,
          output : 'symbols.map',
          command : [meson.source_root()+'/tools/gen-symmap.sh', '@INPUT@', '@OUTPUT@'],
          build_by_default : true)
