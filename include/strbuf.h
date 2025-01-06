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
#define strL(s, l)           { .buf = s, .len = l }
#define strS(s)              { .buf = s, .len = lengthof(s) }
#define strZ                 { .buf = "", .len = 0 }
// clang-format on

typedef struct str {
    char *buf;
    usize len;
} str;

typedef struct strbuf {
    str s;
    usize cap;
} strbuf;

typedef struct strpair {
    str head;
    str tail;
} strpair;

typedef struct strlist strlist;
struct strlist {
    strlist *next;
    str elem;
};

usize strtrim(str s);
strpair strcut(str s, char c);
bool bufextend(strbuf *buf, str s);

#endif // METANG_STRBUF_H
