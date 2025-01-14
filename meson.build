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
    version: run_command(['./tools/version.sh', '.version'], check: true).stdout().strip(),
    default_options: {
        'buildtype': 'release', # O3
        'b_ndebug': 'if-release',
        'warning_level': '2', # all, extra
    },
)

native = get_option('native')
install = native and meson.is_cross_build() ? false : true
version = files('.version')

# This must come before `tests`!
subdir('tools')

subdir('include')
subdir('src')

if not meson.is_subproject()
    subdir('tests')
endif

metang_exe = executable(
    'metang',
    sources: [
        static_sources,
        gen_version_h,
    ],
    include_directories: public_includes,
    install: install,
    install_mode: 'rwxr-xr-x',
    native: native,
)

install_man('docs/metang.1')

meson.override_find_program('metang', metang_exe)
