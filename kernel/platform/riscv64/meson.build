nyy = executable('nyy', 
  'entry.c', kernel_sources,
  c_args: [ freestanding_c_args, '-std=gnu2x'],
  include_directories: [kernel_include_dirs],
  link_args: ['-nostdlib', '-static',
    '-Wl,-T'+meson.current_source_dir()+'/linker.ld'
  ],
  dependencies: [kernel_deps]
)
