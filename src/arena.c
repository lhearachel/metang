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

#include "arena.h"
#include "meta.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void arena_init(arena *a, usize cap)
{
    a->mem = malloc(cap);
    a->cap = a->mem ? cap : 0;
    a->ofs = 0;
}

void arena_free(arena *a, int flags)
{
    if (flags & A_F_ZERO) {
        arena_zero(a, 0, a->cap);
    }

    free(a->mem);
}

void arena_zero(arena *a, usize beg, usize len)
{
    memset(a->mem + beg, 0, len);
}

void *arena_claim(arena *a, usize size, usize align, usize count, int flags)
{
    usize available = a->cap - a->ofs;
    usize padding = a->ofs & (align - 1);

    if (count > PTRDIFF_MAX || available + padding < size * count) {
        if (flags & A_F_SOFTFAIL) {
            return NULL;
        }

        EXCEPT(a->jmpbuf);
    }

    unsigned char *p = a->mem + a->ofs - padding;
    a->ofs += (size * count) - padding;
    return flags & A_F_ZERO ? memset(p, 0, count * size) : p;
}
