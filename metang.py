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
Usage: metang.py COMMAND [OPTIONS] FILE

When passing enumeration members from standard input, specify “-” for FILE.

Commands:
  enum     Generate an integral enumeration.
  mask     Generate a bitmask enumeration.
  help     Display this help text.
  version  Display the version number of this program.

Global Options:
  -L, --lang <LANG>       Generate the enumeration for a target language. If
                          unspecified, generate for the C language.
                          Options: c, py
  -o, --output <OFILE>    Write output to <OFILE>. If unspecified, write to
                          standard output.
  -l, --leader <LEADER>   Use <LEADER> as a prefix for generated symbols.
  -t, --tag-name <NAME>   Use <NAME> as the tag for enums and associated lookup
                          tables. If unspecified, <NAME> will be derived from
                          the input file's basename, minus any extension.
  -g, --guard <GUARD>     Prefix conditional directives with <GUARD>. For
                          example, in C, this will prefix inclusion guards.

When using the “mask” command, the user must mind the following:
  1. The magic values NONE and ANY are automatically prepended and appended
     to user input, respectively. The NONE value is always assigned the value
     0; the ANY value is always assigned the sum of all previous mask indices.
  2. Overrides on assignment values from user input are not permitted. This is
     to ensure that the generated bitmask is contiguous.
  3. As a consequence of (1) and (2), overrides to the starting value are not
     permitted.
"""

from argparse import ArgumentParser
from pathlib import Path

import sys

from metang.generators import c
from metang.lang import Lang
from metang.mode import Mode
from metang.options import Options
from metang.util import snake


def help():
    print("metang - Generate enumerated constants from plain-text")
    print(__doc__)
    sys.exit(0)


def version():
    print("0.1.1")
    sys.exit(0)


COMMANDS = {
    "enum",
    "mask",
    "help",
    "version",
}

if len(sys.argv) == 1 or sys.argv[1] == "help":
    help()

if len(sys.argv) < 3:
    print("metang: Missing required positional argument")
    print(__doc__)
    sys.exit(1)

command = sys.argv[1]
if command not in COMMANDS:
    print("metang: Unrecognized command")
    print(__doc__)
    sys.exit(1)

if command == "help":
    help()
elif command == "version":
    version()

argp = ArgumentParser(prog="metang", usage=__doc__, add_help=False)
argp.add_argument(
    "-L", "--lang", type=Lang.argparse, choices=list(Lang), default=Lang.c
)
argp.add_argument("-o", "--output")
argp.add_argument("-l", "--leader")
argp.add_argument("-t", "--tag-name")
argp.add_argument("-g", "--guard")
argp.add_argument("INFILE")

args = argp.parse_args(sys.argv[2:])

if args.output:
    fout_name = args.output
    fout = open(args.output, "w", encoding="utf-8")
else:
    fout_name = "stdout"
    fout = sys.stdout

if args.INFILE != "-":
    fin_name = args.INFILE
    fin = open(args.INFILE, "r", encoding="utf-8")
else:
    fin_name = "stdin"
    fin = sys.stdin

lang = args.lang
leader = snake(args.leader).upper() if args.leader else ""
tag = args.tag_name if args.tag_name else snake(Path(fin_name).stem).lower()
guard = snake(args.guard).upper() if args.guard else "METANG"
mode = Mode.ENUM if command == "enum" else Mode.MASK

opts = Options(fin, fin_name, fout, fout_name, lang, leader, tag, guard, mode)

enumeration = []
if mode & Mode.MASK:
    enumeration.append((f"{tag}_NONE", -1))

val = 0
maxlen = 0
for line in filter(lambda line: not line.startswith("#"), fin):
    no_comm = line.split("#")[0]
    if mode & Mode.MASK and "=" in no_comm:
        raise ValueError("Direct value-assignment is unsupported for bitmasks")

    split = no_comm.split("=")
    if len(split) > 1:
        val = int(split[1])
    idt = split[0].strip()
    if len(idt) > maxlen:
        maxlen = len(idt)

    enumeration.append((idt, val))
    val += 1

if mode & Mode.MASK:
    enumeration.append((f"{tag}_ALL", val))

digits = len(str(val - 1))

c.generate(enumeration, opts, maxlen, digits)

# for member in enumeration:
#     print(f"{member[0]: <{maxlen}} = {member[1]: >{digits}}")
