nyy = executable('nyy', 
  'entry.c', kernel_sources,
  c_args: [freestanding_c_args,'-std=gnu2x'],
  include_directories: [freestanding_include_dirs],
  link_args: [],
  dependencies: [kernel_deps]
)
