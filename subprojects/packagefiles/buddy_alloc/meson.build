project('buddy_alloc', ['c'])

args = [
]

incs = [
  include_directories('.')
]

buddy_alloc_dep = declare_dependency(
  compile_args: args, 
  include_directories: incs
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('buddy_alloc', buddy_alloc_dep)
endif
