nyy = executable('nyy', 
  'entry.c', 'gdt.c', 'int.c',
  'gdt.S', 'stubs.S',
  kernel_sources,
  c_args: [ freestanding_c_args, '-std=gnu2x'],
  include_directories: [freestanding_include_dirs],
  link_args: ['-nostdlib', '-static', '-lgcc',
    '-Wl,-T'+meson.current_source_dir()+'/linker.ld'
  ],
  dependencies: []
)
