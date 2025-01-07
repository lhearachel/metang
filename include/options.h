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

#ifndef METANG_OPTIONS_H
#define METANG_OPTIONS_H

#include "meta.h"
#include "strbuf.h"

#define MAX_ADDITIONAL_VALS 16

enum tag_case {
    TAG_SNAKE_CASE,
    TAG_PASCAL_CASE,
};

enum result_code {
    OPTS_S,
    OPTS_F_UNRECOGNIZED_OPT,
    OPTS_F_OPT_MISSING_ARG,
    OPTS_F_TOO_MANY_APPENDS,
    OPTS_F_TOO_MANY_PREPENDS,
    OPTS_F_NOT_AN_INTEGER,
    OPTS_F_UNRECOGNIZED_CASING,
    OPTS_F_UNRECOGNIZED_LANG,
};

typedef struct options {
    enum result_code result;
    str last_opt;
    str last_arg;

    str append[MAX_ADDITIONAL_VALS];
    str prepend[MAX_ADDITIONAL_VALS];
    u16 append_count;
    u16 prepend_count;

    isize start;
    str leader;
    str tag;
    str guard;
    str outfile;
    str infile;

    str lang;
    usize genf;

    union {
        struct {
            u32 casing    : 3;
            u32 bitmask   : 1;
            u32 overrides : 1;
            u32 help      : 1;
            u32 version   : 1;
        };
        u32 flags;
    };
} options;

bool parseopts(int *argc, char ***argv, options *opts);
void optserr(options *opts, str *sbuf);

#endif // METANG_OPTIONS_H
