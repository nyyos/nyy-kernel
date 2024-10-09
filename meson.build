project('Nyy')

keyval = import('keyval')
loadedconfig = keyval.load('.config')
conf = configuration_data(loadedconfig)
prog_python = find_program('python3')
autoconf_header = custom_target('gen-autoconf',
  input : [meson.source_root() + '/.config'],
  output : ['autoconf.h'],
  env : {'KCONFIG_CONFIG': meson.source_root() + '/.config', 'ROOT': meson.source_root()},
  command : [prog_python, meson.source_root() + '/vendor/kconfiglib/genconfig.py', '--header-path', '@OUTPUT0@', meson.source_root()+'/Kconfig']
)
autoconf_dep = declare_dependency(sources: autoconf_header)

pairing_heap = subproject('pairing_heap')
uacpi = subproject('uacpi')
uacpi_sources = uacpi.get_variable('sources')
uacpi_includes = uacpi.get_variable('includes')
kernel_deps = [
  autoconf_dep, 
  dependency('flanterm'), 
  dependency('pairing_heap'),
  dependency('freestnd_cxx_hdrs'),
  dependency('freestnd_c_hdrs'),
  dependency('cc_runtime'),
]

config_h = configure_file(
  input: 'config.template',
  output: 'nyyconf.h',
  configuration: conf)

arch = get_option('arch')

add_languages('c', 'cpp')

warnings = [
  '-Wno-comment', # added because of warning in nanoprintf ??
]

freestanding_c_args = [
  '-fno-strict-aliasing',
  '-fwrapv',
  '-fno-delete-null-pointer-checks',
  '-fno-omit-frame-pointer',
  '-nostdinc',
  '-ffreestanding',
  '-fno-PIC',
  '-fno-lto',
  '-ffunction-sections',
  '-fdata-sections',
  '-fvisibility=hidden',
  '-D__KERNEL__',
  warnings,
]

if conf.get('CONFIG_STACKGUARD', 'n') == 'y'
  freestanding_c_args += '-fstack-protector-strong'
else
  freestanding_c_args += ['-fno-stack-protector', '-fno-stack-check']
endif

if conf.get('CONFIG_UBSAN', 'n') == 'y'
  freestanding_c_args += '-fsanitize=undefined' 
endif

use_limine = false

if (arch == 'amd64')
  use_limine = true
  meson_cpu_family = 'x86_64'
  cpu = 'amd64'
  freestanding_c_args += [
    '-m64',
    '-march=x86-64',
    '-mabi=sysv',
    '-mno-80387',
    '-mno-mmx',
    '-mno-sse',
    '-mno-sse2',
    '-mno-red-zone',
    '-mcmodel=kernel',
    '-DAMD64'
  ]
elif (arch == 'aarch64')
  use_limine = true
  meson_cpu_family = 'aarch64'
  cpu = 'aarch64'
  freestanding_c_args += [
    '-mgeneral-regs-only'
  ]
elif (arch == 'riscv64')
  use_limine = true
  meson_cpu_family = 'riscv64'
  cpu = meson_cpu_family
  if meson.get_compiler('c').get_id() == 'clang'
    freestanding_c_args += [
      '-march=rv64imac'
    ]
  else
    freestanding_c_args += [
      '-march=rv64imac_zicsr_zifencei'
    ]
  endif
  freestanding_c_args += [
    '-mabi=lp64',
    '-mno-relax',
    '-mcmodel=medany'
  ]
else
  message('\n\t' + arch + ' not supported by Nyy')
  error('\n\tbad arch')
endif

freestanding_cpp_args = [
  freestanding_c_args,
  '-fno-rtti',
  '-fno-exceptions',
  '-fsized-deallocation',
  '-fno-unwind-tables',
  '-fno-asynchronous-unwind-tables'
]

freestanding_c_args += ['-DARCHNAME="' + arch +'"']

kernel_sources = []
kernel_include_dirs = []

kernel_include_dirs += include_directories(
  'kernel/include',
  'kernel'
)

if(use_limine)
  kernel_include_dirs += include_directories('vendor/limine')
endif

if(use_limine)
  executable(
    'limine',
    'vendor/limine/limine.c',
    native: true
  )
endif

subdir('kernel')
