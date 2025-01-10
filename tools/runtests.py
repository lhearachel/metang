#!/usr/bin/env python3

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

"""
A simple test runner for metang.

Usage: runtests.py [--rewrite] command [names...]

Test names are the name of a file under the `tests` folder, minus the `.test`
extension. If no test names are given as arguments, then all tests will be run.
If any test fails due to unexpected output, then a diff of the actual output
against expected output will be written to the console.

Test files must adhere to the following schema:

    ```
    <arguments to metang>
    # input
    <lines of test input>
    # output
    <lines of expected output>
    ```
"""

import difflib
import pathlib
import re
import subprocess
import sys


TESTS_DIR = pathlib.Path(__file__).parent.parent.resolve() / "tests"
STRIP_ANSI = re.compile("\x1b[^m]+m")
ANSI_R = "\x1b[31m"
ANSI_G = "\x1b[32m"
ANSI_Y = "\x1b[33m"
ANSI_C = "\x1b[0m"


def run_test(command: str, name: str, fix_output: bool = False) -> str | None:
    """
    Load and run a single test.
    """
    target = TESTS_DIR / command / f"{name}.test"
    if not target.exists():
        raise ValueError(f"unrecognized test name: {name}")

    test_args: list[str] = []
    test_stdin: list[str] = []
    test_expect: list[str] = []
    with open(target, "r", encoding="utf-8") as f:
        t = test_args
        for line in f:
            if line == "# input\n":
                t = test_stdin
                continue
            elif line == "# output\n":
                t = test_expect
                continue
            t.append(line)

    args = list(
        filter(lambda s: s, "".join(test_args).replace("\n", " ").strip().split(" "))
    )
    result = subprocess.run(
        [
            "./metang",
            command,
            *args,
        ],
        input="".join(test_stdin),
        capture_output=True,
        encoding="utf-8",
    )

    output_lines = [
        STRIP_ANSI.sub("", line)
        for line in result.stdout.splitlines() + result.stderr.splitlines()
    ]

    if fix_output:
        new_output = "".join(
            [
                *test_args,
                "# input\n",
                *test_stdin,
                "# output\n",
                "\n".join(output_lines),
                "\n",
            ]
        )
        with open(target, "w", encoding="utf-8") as f:
            f.write(new_output)
        return None

    report = []
    diff = difflib.unified_diff(
        a=output_lines,
        b=[line[:-1] for line in test_expect],
        fromfile=f"actual: {target.parent.name} - {target.stem}",
        tofile=f"expect: {target.parent.name} - {target.stem}",
        lineterm="",
    )
    for line in diff:
        if line.startswith("-"):
            report.append(ANSI_R + line + ANSI_C)
        elif line.startswith("+"):
            report.append(ANSI_G + line + ANSI_C)
        else:
            report.append(line)

    return "\n".join(report) if report else None


argv = sys.argv[1:]
rewrite = False
if argv[0] == "--rewrite":
    rewrite = True
    argv = argv[1:]

command = argv[0]

if len(argv) > 1:
    tests = argv[1:]
else:
    tests = list(map(lambda p: p.stem, (TESTS_DIR / command).glob("*.test")))

exit_code = 0
for test in tests:
    if result := run_test(command, test, rewrite):
        print(f"{test}:\n{result}")
        exit_code = 1
    else:
        print(f"{ANSI_G}✔{ANSI_C}  {ANSI_Y}{command} - {test}{ANSI_C}")
sys.exit(exit_code)
