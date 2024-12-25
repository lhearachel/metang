# Copyright 2024 <lhearachel@proton.me>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(
    'metang',
    'c',
    license: 'Apache-2.0',
    license_files: 'LICENSE',
    meson_version: '>=1.5.0',
    version: '0.1.0',
)

native = get_option('native')
install = native and meson.is_cross_build() ? false : true

c_sources = files(
    'src/deque.c',
    'src/generate.c',
    'src/metang.c',
    'src/strlib.c',
)

includes = include_directories('include')

metang_exe = executable(
    'metang',
    c_sources,
    c_args: [
        '-MMD',
        '-Wall',
        '-Wextra',
        '-Wpedantic',
        '-std=c99',
        '-O2',
    ],
    include_directories: includes,
    install: install,
    install_dir: get_option('install_dir'),
    install_mode: 'rwxrw-rw-',
    native: native,
)

metang_runtests = find_program('tools/runtests.py', native: true)
test('Regression Tests', metang_runtests)

meson.override_find_program('metang', metang_exe)
