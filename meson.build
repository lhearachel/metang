project(
    'metang',
    license: 'Apache-2.0',
    license_files: 'LICENSE',
    meson_version: '>=1.5.0',
)

metang_py = find_program('metang.py', native: true)
meson.override_find_program('metang', metang_py)
