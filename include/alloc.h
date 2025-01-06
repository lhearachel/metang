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

#ifndef METANG_ALLOC_H
#define METANG_ALLOC_H

#include "meta.h"

#include <setjmp.h>

#define new(...)                 newX(__VA_ARGS__, newF, newN, newT, newA)(__VA_ARGS__)
#define newX(a, b, c, d, e, ...) e
#define newA(a)                  a
#define newT(a, T)               (T *)alloc(a, sizeof(T), alignof(T), 1, 0)
#define newN(a, T, n)            (T *)alloc(a, sizeof(T), alignof(T), n, 0)
#define newF(a, T, n, f)         (T *)alloc(a, sizeof(T), alignof(T), n, f)

#define A_F_ZERO      (1 << 0)
#define A_F_SOFT_FAIL (1 << 1)
#define A_F_EXTEND    (1 << 2)

typedef struct arena {
    char *mem;   // Internal memory block
    usize cap;   // Total memory capacity of the block
    usize ofs;   // Current "head" offset in the block
    jmp_buf env; // Jump buffer to be set by a calling client; refer to `alloc` for details
} arena;

arena arena_new(usize cap);

// Expose the next offset at which internal memory will be allocated.
usize nextofs(arena *a, usize align);

// Allocate memory from an arena `a` for `n` members with `size` and `align`
// and return an address to the allocated block. If the allocation fails, then
// control will be returned to the client via a long jump on `a->env`.
//
// Flags specified control the internal behavior:
//   - `A_F_ZERO`      -> The requested memory will be zeroed on return.
//   - `A_F_SOFT_FAIL` -> If allocation fails, then return `NULL` and do not jump.
//   - `A_F_EXTEND`    -> If allocation fails, first attempt to reallocate the
//                        underlying buffer. If successful, proceed as normal.
void *alloc(arena *a, usize size, usize align, usize n, int flags);

// Claim a block of memory `buf` with size `len` as part of arena `a`. Return
// the address of the start of the claimed block on success, otherwise return
// control to the client via a long jump on `a->env`.
//
// If `A_F_SOFT_FAIL` is specified and `a` as insufficient available memory to
// claim `buf`, then return `NULL` rather than executing the long jump.
//
// If `A_F_EXTEND` is specified and `a` has insufficient available memory to
// claim `buf`, then this routine will attempt to reallocate `a`'s available
// memory space to fit. If this reallocation fails, then the failure strategy
// defers to the existence of `A_F_SOFT_FAIL`.
void *claim(arena *a, char *buf, usize len, int flags);

#endif // METANG_ALLOC_H
