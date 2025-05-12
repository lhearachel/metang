man_1_adoc_files = [
  'metang.adoc',
]

asciidoctor_exe = find_program('asciidoctor')

man_1 = []
foreach man_1_adoc : man_1_adoc_files
  man_1_out = man_1_adoc.replace('.adoc', '.1')
  man_1 += custom_target(
    input: files(man_1_adoc),
    output: man_1_out,
    command: [
      asciidoctor_exe,
      '-b', 'manpage',
      '-a', 'manname=' + meson.project_name(),
      '-a', 'manversion=' + meson.project_version(),
      '-a', 'docdate=2025-05-11',
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    install: install,
    install_dir: ['share/man/man1'],
    install_tag: ['man'],
  )
endforeach

alias_target('manuals', man_1)
