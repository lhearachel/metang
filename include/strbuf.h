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

#ifndef METANG_STRBUF_H
#define METANG_STRBUF_H

#include "meta.h"

// clang-format off
#define strnew(...)          strX(__VA_ARGS__, strL, strS, strZ)(__VA_ARGS__)
#define strX(a, b, c, ...)   c
#define strL(s, l)           (str){ .buf = s, .len = l }
#define strS(s)              (str){ .buf = s, .len = lengthof(s) }
#define strZ                 (str){ .buf = "", .len = 0 }
#define strnewp(s)           (str){ .buf = s->buf, .len = s->len }
// clang-format on

typedef struct str {
    char *buf;
    usize len;
} str;

typedef struct strpair {
    str head;
    str tail;
} strpair;

typedef struct strlist strlist;
struct strlist {
    strlist *next;
    str elem;
};

bool streq(const str *s1, const str *s2);

// Compute the length of a string with trailing whitespace removed.
usize strtrim(const str *s);

// Cut `str` into halves by a delimiter `c`, traversing front-to-back.
strpair strcut(const str *s, char c);

// Cut `str` into halves by a delimiter `c`, traversing back-to-front.
strpair strrcut(const str *s, char c);

// Like `strcut`, but chops all leading characters matching `c`, then returns
// a `str` starting from the first non-`c` character.
str strchop(const str *s, char c);

bool strtolong(const str *s, long *l);

#endif // METANG_STRBUF_H
