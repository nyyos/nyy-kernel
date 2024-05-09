project('Nyy')

arch = get_option('arch')

add_languages('c')

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
]

if get_option('ubsan')
  freestanding_c_args += '-fsanitize=undefined' 
endif

if (arch == 'amd64')
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
  meson_cpu_family = 'aarch64'
  cpu = 'aarch64'
  freestanding_c_args += [
    '-mgeneral-regs-only'
  ]
elif (arch == 'riscv64')
  meson_cpu_family = 'riscv64'
  cpu = meson_cpu_family
  freestanding_c_args += [
    '-march=rv64imac'
  ]
else
  message('\n\t' + arch + ' not supported by Nyy')
  error('\n\tbad arch')
endif

freestanding_include_dirs = [ include_directories(
  'vendor/limine',
  'kernel/include',
  'kernel'
)]

executable(
  'limine',
  'vendor/limine/limine.c',
  native: true
)

subdir('kernel')
if (arch == 'amd64')
  subdir('platform/amd64')
elif (arch == 'riscv64')
  subdir('platform/riscv64-virt')
endif
