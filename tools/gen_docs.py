#!/usr/bin/env python3

# Copyright 2025 <lhearachel@proton.me>
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

"""
A documentation generator for metang.
"""

import subprocess
import sys

readme_template = sys.argv[1]
version = sys.argv[2]
enum_base_test = sys.argv[3]
enum_base_py_test = sys.argv[4]
metang_exe = f"./{sys.argv[5]}"
man_md = sys.argv[6]

with open(version, "r", encoding="utf-8") as version_f:
    version_s = version_f.read()

with open(readme_template, "r", encoding="utf-8") as readme_template_f:
    readme = readme_template_f.read()

enum_base_input_lines = []
enum_base_output_lines = []

with open(enum_base_test, "r", encoding="utf-8") as enum_base_test_f:
    target = None
    for line in enum_base_test_f:
        if line == "# input\n":
            target = enum_base_input_lines
        elif line == "# output\n":
            target = enum_base_output_lines
        elif target is not None:
            target.append(line.rstrip())

enum_base_py_output_lines = []

with open(enum_base_py_test, "r", encoding="utf-8") as enum_base_py_test_f:
    target = None
    for line in enum_base_py_test_f:
        if line == "# output\n":
            target = enum_base_py_output_lines
        elif target is not None:
            target.append(line.rstrip())

help_result = subprocess.run(
    [metang_exe, "help"], capture_output=True, encoding="utf-8"
)

man_result = subprocess.run(
    ["md2man-roff", man_md], capture_output=True, encoding="utf-8"
)

readme = readme.replace("{{ VERSION }}\n", version_s)
readme = readme.replace("{{ ENUM_BASE_INPUT }}", "\n".join(enum_base_input_lines))
readme = readme.replace("{{ ENUM_BASE_OUTPUT }}", "\n".join(enum_base_output_lines))
readme = readme.replace(
    "{{ ENUM_BASE_PY_OUTPUT }}", "\n".join(enum_base_py_output_lines)
)
readme = readme.replace("{{ HELPTEXT }}", help_result.stdout)

with open("README.md", "w", encoding="utf-8") as readme_out:
    readme_out.write(readme)

with open("docs/metang.1", "w", encoding="utf-8") as man_out:
    man_out.write(man_result.stdout)
