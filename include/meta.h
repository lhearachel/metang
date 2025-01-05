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

#ifndef METANG_META_H
#define METANG_META_H

#include <stddef.h>
#include <stdint.h>

#define alignof(a)  _Alignof(a)
#define sizeof(x)   (ptrdiff_t)sizeof(x)
#define countof(a)  (sizeof(a) / sizeof(*(a)))
#define lengthof(s) (countof(s) - 1)

#define false 0
#define true  1

// clang-format off
typedef uint8_t         u8;
typedef uint16_t        u16;
typedef uint32_t        u32;
typedef int8_t          i8;
typedef int16_t         i16;
typedef int32_t         i32;
typedef intptr_t        isize;
typedef uintptr_t       usize;

typedef int32_t         bool;

typedef unsigned char   byte;
// clang-format on

#define CATCH(jb, expr)         \
    void *jb[5];                \
    if (__builtin_setjmp(jb)) { \
        expr;                   \
    }

#define EXCEPT(jb) __builtin_longjmp(jb, 1)

#endif // METANG_META_H
