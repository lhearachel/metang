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

#include "alloc.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

arena arena_new(usize cap)
{
    arena a = {0};
    a.mem = malloc(cap);
    a.cap = cap;
    a.ofs = 0;
    return a;
}

arena arena_from(char *mem, usize cap)
{
    return (arena){
        .mem = mem,
        .cap = cap,
        .ofs = 0,
    };
}

usize nextofs(arena *a, usize align)
{
    // Exploit two's complement to get the padding; to illustrate, suppose
    // that you have a buffer of 32 bytes, of which 10 bytes have been allocated
    // on a platform with 8-byte alignment. The goal is to reach offset 16 for
    // the next allocation.
    //
    //  10 & (8 - 1) => 1010 & 0111 => 0010 (decimal 2)
    // -10 & (8 - 1) => 0110 & 0111 => 0110 (decimal 6)
    usize padding = -a->ofs & (align - 1);
    return a->ofs + padding;
}

void *alloc(arena *a, usize size, usize align, usize n, int flags)
{
    usize next = nextofs(a, align);
    usize remaining = a->cap - next;
    usize req_size = n * size;
    void *p;

    if (next > a->cap || req_size > remaining) {
        if (flags & A_F_EXTEND) {
            usize tcap = (a->cap + req_size) * 2;
            char *tmem = realloc(a->mem, tcap);
            if (tmem != NULL) {
                a->mem = tmem;
                a->cap = tcap;
                goto advance;
            }
        }

        if (flags & A_F_SOFT_FAIL) {
            return NULL;
        }

        longjmp(a->env, 1);
    }

advance:
    p = a->mem + next;
    a->ofs = next + req_size;
    return flags & A_F_ZERO ? memset(p, 0, req_size) : p;
}

void *claim(arena *a, char *buf, usize len, int flags)
{
    void *p = alloc(a, sizeof(char), alignof(char), len, flags);
    return p ? memcpy(p, buf, len) : NULL;
}

void pop(arena *a, void *p, usize len, int flags)
{
    if (flags & A_F_ZERO) {
        memset(p, 0, len);
    }

    a->ofs = a->ofs - len;
}
