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

flanterm = subproject('flanterm')
kernel_deps = [autoconf_dep, flanterm.get_variable('flanterm_dep')]

config_h = configure_file(
  input: 'config.template',
  output: 'nyyconf.h',
  configuration: conf)

arch = get_option('arch')

add_languages('c')

warnings = [
  '-Wno-comment', # added because of warning in nanoprintf ??
]

freestanding_c_args = [
  '-fno-strict-aliasing',
  '-fwrapv',
  '-fno-delete-null-pointer-checks',
  '-fno-omit-frame-pointer',
  '-ffreestanding',
  '-fno-stack-protector',
  '-fno-stack-check',
  '-fno-pie',
  '-fno-pic',
  warnings,
]

if conf.get('CONFIG_UBSAN', 'n') == 'y'
  freestanding_c_args += '-fsanitize=undefined' 
endif

build_limine = false

if (arch == 'amd64')
  build_limine = true
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
  build_limine = true
  meson_cpu_family = 'aarch64'
  cpu = 'aarch64'
  freestanding_c_args += [
    '-mgeneral-regs-only'
  ]
elif (arch == 'riscv64')
  build_limine = true
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
elif (arch == 'linux')
  freestanding_c_args = [
    warnings,
    '-DNYYLINUX'
  ]
else
  message('\n\t' + arch + ' not supported by Nyy')
  error('\n\tbad arch')
endif

freestanding_include_dirs = [ include_directories(
  'kernel/include',
  'kernel'
)]

if(build_limine)
  freestanding_include_dirs += [include_directories('vendor/limine')]
endif

if(build_limine)
  executable(
    'limine',
    'vendor/limine/limine.c',
    native: true
  )
endif

subdir('kernel')
if (arch == 'amd64')
  subdir('platform/amd64')
elif (arch == 'riscv64')
  subdir('platform/riscv64-virt')
elif (arch == 'linux')
  subdir('platform/linux')
endif
