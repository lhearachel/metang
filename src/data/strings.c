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

const str tag_line = strnew("metang - Generate enumerated constants from plain-text");

const str short_usage = strnew("Usage: metang COMMAND [OPTIONS] [<FILE>]");

const str commands_section = strnew(""
    "Commands:\n"
    "  enum     Generate an integral enumeration.\n"
    "  mask     Generate a bitmask enumeration.\n"
    "  help     Display this help text.\n"
    "  version  Display the version number of this program."
    "");

const str global_options_section = strnew(""
    "Global Options:\n"
    "  -L, --lang <LANG>        Generate the enumeration for a target language.\n"
    "                           If unspecified, generate for the C language.\n"
    "                           Options: c, py\n"
    "  -o, --output <OFILE>     Write output to <OFILE>.\n"
    "                           If unspecified, write to standard output.\n"
    "  -l, --leader <LEADER>    Use <LEADER> as a prefix for generated symbols.\n"
    "  -t, --tag-name <NAME>    Use <NAME> as the base tag for enums and lookup\n"
    "                           tables.\n"
    "                           If unspecified, <NAME> will be derived from the\n"
    "                           input file's basename, minus any extension.\n"
    "  -G, --guard <GUARD>      Prefix conditional directives with <GUARD>. For\n"
    "                           example, in C, this will prefix inclusion guards."
    "");

const str enum_options_section = strnew(""
    "When using the “enum” command, the following additional options are supported:\n"
    "  -a, --append <ENTRY>         Append <ENTRY> to the input listing.\n"
    "  -p, --prepend <ENTRY>        Prepend <ENTRY> to the input listing.\n"
    "  -n, --start-from <NUMBER>    Start enumeration from <NUMBER>.\n"
    "");

const str mask_notes_section = strnew(""
    "When using the “mask” command, the user should mind the following:\n"
    "  1. The magic values NONE and ANY are automatically prepended and appended\n"
    "     to user input, respectively. The NONE value is always assigned the value\n"
    "     0; the ANY value is always assigned the sum of all previous mask indices.\n"
    "  2. Overrides on assignment values from user input are not permitted. This is\n"
    "     to ensure that the generated bitmask is contiguous.\n"
    "  3. As a consequence of (1) and (2), overrides to the starting value are not\n"
    "     permitted."
    "");

const str header_warning = strnew("This file was generated by metang; DO NOT MODIFY IT!!");
const str header_source_file = strnew("Source file: ");
const str header_program_opts = strnew("Program options: ");
// clang-format on
