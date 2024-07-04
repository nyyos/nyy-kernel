project('flanterm', ['c'], license: 'BSD')

args = [
'-DFLANTERM_FB_DISABLE_CANVAS',
# '-DFLANTERM_FB_DISABLE_BUMP_ALLOC'
]

src = [
  'flanterm.c',
  'backends/fb.c'
]

incs = [
  include_directories('.')
]

flanterm_dep = declare_dependency(
  compile_args: args, 
  include_directories: incs, 
  sources: src
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('flanterm', flanterm_dep)
endif
