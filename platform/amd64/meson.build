nyy = executable('nyy', 
  'entry.c', 'gdt.c', 'int.c',
  'gdt.S', 'stubs.S', 'vm.c',
  'string.S',
  kernel_sources,
  c_args: [ freestanding_c_args, '-std=gnu2x'],
  include_directories: [freestanding_include_dirs],
  link_args: ['-Wl,-pie','-nostdlib', '-static',
    '-Wl,-T'+meson.current_source_dir()+'/linker.ld'
  ],
  dependencies: [
    kernel_deps
  ]
)

custom_target('generate symbols',
          depends : nyy,
          input : nyy,
          output : 'symbols.map',
          command : [meson.source_root()+'/tools/gen-symmap.sh', '@INPUT@', '@OUTPUT@'],
          build_by_default : true)
