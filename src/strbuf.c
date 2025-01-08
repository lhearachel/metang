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

#include "alloc.h"

bool streq(const str *s1, const str *s2)
{
    return s1->len == s2->len && strncmp(s1->buf, s2->buf, s1->len) == 0;
}

bool strhas(const str *s, char c)
{
    for (usize i = 0; i < s->len; i++) {
        if (s->buf[i] == c) {
            return true;
        }
    }

    return c == '\0';
}

bool strhasany(const str *s1, const str *s2)
{
    for (usize i = 0; i < s1->len; i++) {
        for (usize j = 0; j < s2->len; j++) {
            if (s1->buf[i] == s2->buf[j]) {
                return true;
            }
        }
    }

    return false;
}

str strclone(const str *s, arena *a, int flags)
{
    return strnew(claim(a, s->buf, s->len + 1, flags), s->len);
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
    pair.tail.len = (pair.head.len == s->len)
        ? 0
        : s->len - pair.head.len - 1;
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

bool strtolong(const str *s, long *l)
{
    bool neg = false;
    usize i = 0;
    if (s->buf[0] == '-') {
        neg = true;
        i++;
    }

    *l = 0;
    for (; i < s->len; i++) {
        if (s->buf[i] >= '0' && s->buf[i] <= '9') {
            *l = (*l * 10) + (s->buf[i] - '0');
        } else {
            return false;
        }
    }

    if (neg) {
        *l = *l * -1;
    }
    return true;
}

str strsnake(const str *s, char *buf, const str *extrapunc, bool upper)
{
    char *p = buf;
    for (usize i = 0; i < s->len; i++) {
        char c = s->buf[i];
        if (c == '-' || c == '_' || isspace(c)) {
            *p = '_';
            p++;
        } else if (extrapunc && strhas(extrapunc, c)) {
            *p = '_';
            p++;
        } else if (c >= '0' && c <= '9') {
            *p = c;
            p++;
        } else if (upper) {
            if (c >= 'a' && c <= 'z') {
                *p = c - ('a' - 'A');
                p++;
            } else if (c >= 'A' && c <= 'Z') {
                *p = c;
                p++;
            }
        } else {
            if (c >= 'a' && c <= 'z') {
                *p = c;
                p++;
            } else if (c >= 'A' && c <= 'Z') {
                *p = c + ('a' - 'A');
                p++;
            }
        }
    }

    *p = '\0';
    return strnew(buf, strlen(buf));
}
