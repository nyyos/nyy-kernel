ultra_protocol = dependency('ultra_protocol')

kernel_sources += files(
  'entry.c', 
  'gdt.c', 
  'int.c',
  'vm.c',
  'ultra.c', 
  'pic.c', 
  'hpet.c', 
  'pci.c',
  'apic.c', 
  'sched.c',
  'gdt.S', 
  'sched.S', 
  'start.S',
  'stubs.S', 
  'string.S'
)

nyy = executable('nyy', 
  kernel_sources,
  c_args: [ freestanding_c_args, '-std=gnu2x'],
  cpp_args: [ freestanding_cpp_args, '-std=gnu++23' ],
  include_directories: [kernel_include_dirs],
  link_args: [
    '-nostdlib', 
    '-static',
    '-Wl,-gc-sections',
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
