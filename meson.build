project('Nyy')

arch = get_option('arch')

add_languages('c')

freestanding_c_args = [
        '-fno-strict-aliasing',
        '-fwrapv',
        '-fno-delete-null-pointer-checks',
        '-fno-omit-frame-pointer',
]

if (arch == 'amd64')
  meson_cpu_family = 'x86_64'
  cpu = 'amd64'
  freestanding_c_args += [
    '-ffreestanding',
    '-fno-stack-protector',
    '-fno-stack-check',
    '-fno-pie',
    '-fno-pic',
    '-m64',
    '-march=x86-64',
    '-mabi=sysv',
    '-mno-80387',
    '-mno-mmx',
    '-mno-sse',
    '-mno-sse2',
    '-mno-red-zone',
    '-mcmodel=kernel',
  ]
elif (arch == 'aarch64')
  meson_cpu_family = 'aarch64'
  cpu = 'aarch64'
  freestanding_c_args += [
    '-ffreestanding',
    '-fno-stack-protector',
    '-fno-stack-check',
    '-fno-pie',
    '-fno-pic',
    '-mgeneral-regs-only'
  ]
else
  message('\n\t' + arch + ' not supported by Nyy')
  error('\n\tbad arch')
endif

freestanding_include_dirs = [ include_directories(
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
endif
