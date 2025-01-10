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

#ifndef METANG_GENERATE_H
#define METANG_GENERATE_H

#include <stdio.h>

#include "options.h"
#include "strbuf.h"

typedef struct enumerator enumerator;
struct enumerator {
    enumerator *next;
    str ident;
    isize assignment;
    usize count;
    usize max_ident_len;
    usize max_assign_len;
};

typedef bool (*generator_func)(enumerator *input, options *opts, FILE *fout);

typedef struct generator {
    str lang;
    generator_func genfunc;
} generator;

extern const generator generators[];
extern const str header_warning;
extern const str header_source_file;
extern const str header_program_opts;

#endif // METANG_GENERATE_H
