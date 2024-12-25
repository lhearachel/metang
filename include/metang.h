/*
 * Copyright 2024 <lhearachel@proton.me>
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

#ifndef METANG_H
#define METANG_H

#include <stdbool.h>
#include <stdint.h>

#include "deque.h"

typedef intptr_t ssize_t;

struct options {
    struct deque *append;
    struct deque *prepend;
    ssize_t start_from;
    const char *leader;
    const char *preproc_guard;
    const char *output_file;
    const char *input_file;
    bool allow_overrides;
    bool to_stdout;
    bool from_stdin;
};

struct enumerator {
    const char *lvalue;
    long rvalue;
    bool direct;
};

#endif // METANG_H
