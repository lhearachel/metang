project(
  'metang',
  'c',
  version: '0.1.0',
  license: 'MIT',
  license_files: 'LICENSE',
  meson_version: '>=1.5.0',
  default_options: {
    'buildtype': 'debug',
    'warning_level': '3',
    'werror': true,
    'b_sanitize': 'address',
    'b_ndebug': 'if-release',
    'c_std': 'c99',
  }
)

metang_conf = configuration_data()
metang_conf.set_quoted('PROGRAM_NAME', meson.project_name())
metang_conf.set_quoted('PROGRAM_VERSION', meson.project_version())
configure_file(output: 'metang.h', configuration: metang_conf)

native = get_option('native')
install = native and meson.is_cross_build() ? false : true

metang_exe = executable(
  'metang',
  sources: files(
    'source/main.c',
    'source/langs/c.c',
    'source/langs/cpp.c',
    'source/langs/py.c',
    'source/libs/clip.c',
    'source/libs/strings.c',
  ),
  implicit_include_directories: true,
  include_directories: include_directories('include'),
  install: install,
  native: native,
)

if not meson.is_subproject()
  if get_option('manuals')
    subdir('docs')
  endif
else
  meson.override_find_program('metang', metang_exe)
endif
