project('ultra_protocol', ['c'], license: 'MIT')

incs = [
  include_directories('.')
]

ultra_protocol_dep = declare_dependency(
  include_directories: incs, 
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('ultra_protocol', ultra_protocol_dep)
endif
