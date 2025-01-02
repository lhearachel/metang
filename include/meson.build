public_includes = include_directories('.')

gen_version_h = custom_target(
    'version.h',
    output: 'version.h',
    command: [
        metang_version,
        version,
        '@OUTPUT@',
    ],
)
