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

#ifndef METANG_ARENA_H
#define METANG_ARENA_H

#include "meta.h"

// Allocate a new memory chunk on a given allocator, casting it to the
// specified Type. Optionally, N chunks can be allocated, and a set of Flags
// can be passed to the allocator to control internal allocation behavior.
#define new(...)                 newx(__VA_ARGS__, newF, newN, newT, NULL)(__VA_ARGS__)
#define newx(a, b, c, d, e, ...) e
#define newT(a, T)               (T *)arena_claim(a, sizeof(T), alignof(T), 1, 0)
#define newN(a, T, N)            (T *)arena_claim(a, sizeof(T), alignof(T), N, 0)
#define newF(a, T, N, F)         (T *)arena_claim(a, sizeof(T), alignof(T), N, F)

#define A_F_ZERO     (1 << 0) // Request claimed memory to be zeroed out.
#define A_F_SOFTFAIL (1 << 1) // Request to return NULL when failure-to-claim occurs.

typedef struct {
    unsigned char *mem;
    usize cap;
    usize ofs;
    void **jmpbuf;
} arena;

void arena_init(arena *a, usize cap);
void arena_free(arena *a, int flags);
void arena_zero(arena *a, usize beg, usize len);
void *arena_claim(arena *a, usize size, usize align, usize count, int flags);

#endif // METANG_ARENA_H
