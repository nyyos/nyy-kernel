project('cc-runtime', ['c'])

src = [
  'cc-runtime.c',
]

cc_runtime_dep = declare_dependency(
  sources: src
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('cc_runtime', cc_runtime_dep)
endif

