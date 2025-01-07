/*
 * Copyright 2025 <lhearachel@proton.me>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "strbuf.h"
#include "version.h"

// clang-format off
const str version = strnew(METANG_VERSION);

const str tag_line = strnew("metang - Generate multi-purpose C headers for enumerators");

const str short_usage = strnew("Usage: metang [options] [<input_file>]");

const str options_section = strnew(""
    "Options:\n"
    "  -a, --append <entry>         Append <entry> to the input listing.\n"
    "  -p, --prepend <entry>        Prepend <entry> to the input listing.\n"
    "  -n, --start-from <number>    Start enumeration from <number>.\n"
    "  -o, --output <file>          Write output to <file>.\n"
    "  -l, --leader <leader>        Use <leader> as a prefix for generated symbols.\n"
    "  -t, --tag-name <name>        Use <name> as the base tag for enums and lookup\n"
    "                               tables.\n"
    "  -c, --tag-case <case>        Customize the casing of generated tags for enums\n"
    "                               and lookup tables. Options: snake, pascal\n"
    "  -G, --preproc-guard <guard>  Use <guard> as a prefix for conditional\n"
    "                               preprocessor directives.\n"
    "  -L, --lang <lang>            Specify a target langauge for output generation.\n"
    "  -B, --bitmask                If specified, generate symbols for a bitmask.\n"
    "                               This option is incompatible with the “-D” and\n"
    "                               “-n” options.\n"
    "  -D, --allow-overrides        If specified, allow direct value-assignment.\n"
    "  -h, --help                   Display this help text and exit.\n"
    "  -v, --version                Display the program version number and exit."
    "");
// clang-format on
