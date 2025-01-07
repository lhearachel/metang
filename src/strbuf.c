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

#include <stdlib.h>
#include <string.h>

bool streq(const str *s1, const str *s2)
{
    return s1->len == s2->len && strncmp(s1->buf, s2->buf, s1->len) == 0;
}

static inline int isspace(int c)
{
    return c == ' '
        || c == '\t'
        || c == '\n'
        || c == '\v'
        || c == '\f'
        || c == '\r';
}

usize strtrim(const str *s)
{
    usize i = s->len;
    while (i > 0 && isspace(s->buf[i - 1])) {
        i--;
    }
    return i;
}

strpair strcut(const str *s, char c)
{
    strpair pair = {0};
    pair.head.buf = s->buf;

    while (pair.head.len < s->len
           && s->buf[pair.head.len]
           && s->buf[pair.head.len] != c) {
        pair.head.len++;
    }

    pair.tail.buf = s->buf + pair.head.len + 1;
    pair.tail.len = s->len - pair.head.len - 1;
    return pair;
}

strpair strrcut(const str *s, char c)
{
    strpair pair = {0};
    pair.head.buf = s->buf;
    pair.head.len = s->len;

    while (pair.head.len > 0
           && s->buf[pair.head.len - 1]
           && s->buf[pair.head.len - 1] != c) {
        pair.head.len--;
    }

    pair.tail.buf = s->buf + pair.head.len;
    pair.tail.len = s->len - pair.head.len;
    if (pair.head.len != 0) {
        pair.head.len--;
    }

    return pair;
}

str strchop(const str *s, char c)
{
    str chop = {0};
    chop.buf = s->buf;
    chop.len = s->len;

    while (*chop.buf == c) {
        chop.buf++;
        chop.len--;
    }

    return chop;
}
