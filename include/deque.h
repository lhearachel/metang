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

#ifndef METANG_STRDEQUE_H
#define METANG_STRDEQUE_H

#include <stddef.h>

struct deque;
struct deque_node;

struct deque *deque_new(void);
void deque_clear(struct deque *deque);
void deque_free(struct deque *deque);

struct deque_node *deque_push_b(struct deque *deque, void *data);
struct deque_node *deque_push_f(struct deque *deque, void *data);
struct deque_node *deque_pop_b(struct deque *deque);
struct deque_node *deque_pop_f(struct deque *deque);

void deque_foreach_ftob(struct deque *deque, void (*func)(void *data));
void deque_foreach_btof(struct deque *deque, void (*func)(void *data));

#endif // METANG_STRDEQUE_H