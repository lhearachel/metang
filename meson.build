project(
  'metang',
  'c',
  version: '0.1.0',
  license: 'MIT',
  license_files: 'LICENSE',
  meson_version: '>=1.5.0',
  default_options: {
    'warning_level': '3',
    'werror': true,
    'c_std': 'c99',
  }
)

metang_conf = configuration_data()
metang_conf.set_quoted('PROGRAM_NAME', meson.project_name())
metang_conf.set_quoted('PROGRAM_VERSION', meson.project_version())
configure_file(output: 'metang.h', configuration: metang_conf)

executable(
  'metang',
  sources: files(
    'source/main.c',
    'source/langs/c.c',
    'source/libs/clip.c',
    'source/libs/strings.c',
  ),
  implicit_include_directories: true,
  include_directories: include_directories('include'),
)
