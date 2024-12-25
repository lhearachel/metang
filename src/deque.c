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

#include "deque.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum deque_iter_dir {
    ITER_DIR_TO_HEAD = 0,
    ITER_DIR_TO_TAIL,
    ITER_DIR_MAX,
};

struct deque_node {
    void *data;
    struct deque_node *neighbors[ITER_DIR_MAX];
};

struct deque {
    struct deque_node *ptrs[ITER_DIR_MAX];
    size_t size;
};

#define prev neighbors[ITER_DIR_TO_HEAD]
#define next neighbors[ITER_DIR_TO_TAIL]
#define head ptrs[ITER_DIR_TO_HEAD]
#define tail ptrs[ITER_DIR_TO_TAIL]

struct deque *deque_new(void)
{
    return calloc(1, sizeof(struct deque));
}

void deque_clear(struct deque *deque, void (*data_free_func)(void *data))
{
    while (deque->head != NULL) {
        struct deque_node *node = deque->head->next;
        data_free_func(deque->head->data);
        free(deque->head);
        deque->head = node;
        deque->size--;
    }

    deque->tail = NULL;
}

void deque_free(struct deque *deque, void (*data_free_func)(void *data))
{
    deque_clear(deque, data_free_func);
    free(deque);
}

static struct deque_node *deque_push(struct deque *deque, void *data, enum deque_iter_dir dir)
{
    struct deque_node *node = calloc(1, sizeof(struct deque_node));
    if (node == NULL) {
        return NULL;
    }

    node->data = data;

    // deque is empty -> node is both head and tail
    if (deque->size == 0) {
        deque->head = node;
        deque->tail = node;
        deque->size = 1;
        return node;
    }

    node->neighbors[!dir] = deque->ptrs[dir]; // set new node's prev/next to tail/head, respectively
    deque->ptrs[dir]->neighbors[dir] = node;  // set the neighbor of the head/tail to the new node
    deque->ptrs[dir] = node;                  // set the head/tail of the deque to the new node
    deque->size++;
    return node;
}

struct deque_node *deque_push_b(struct deque *deque, void *data)
{
    return deque_push(deque, data, ITER_DIR_TO_TAIL);
}

struct deque_node *deque_push_f(struct deque *deque, void *data)
{
    return deque_push(deque, data, ITER_DIR_TO_HEAD);
}

static struct deque_node *deque_pop(struct deque *deque, enum deque_iter_dir dir)
{
    if (deque->size == 0) {
        return NULL;
    }

    struct deque_node *pop = deque->ptrs[dir]; // grab the head/tail
    deque->ptrs[dir] = pop->neighbors[!dir];   // set the head/tail to popped node's next/prev, respectively
    pop->neighbors[!dir] = NULL;               // zero out the popped node's next/prev
    return pop;
}

struct deque_node *deque_pop_b(struct deque *deque)
{
    return deque_pop(deque, ITER_DIR_TO_TAIL);
}

struct deque_node *deque_pop_f(struct deque *deque)
{
    return deque_pop(deque, ITER_DIR_TO_HEAD);
}

void *deque_peek_b(struct deque *deque)
{
    return deque->tail->data;
}

void *deque_peek_f(struct deque *deque)
{
    return deque->head->data;
}

void deque_foreach(struct deque *deque, void (*func)(void *data, void *user), void *user, enum deque_iter_dir dir)
{
    if (deque->size == 0) {
        return;
    }

    struct deque_node *cursor = deque->ptrs[!dir];
    do {
        func(cursor->data, user);
        cursor = cursor->neighbors[dir];
    } while (cursor != NULL);
}

void deque_foreach_ftob(struct deque *deque, void (*func)(void *data, void *user), void *user)
{
    deque_foreach(deque, func, user, ITER_DIR_TO_TAIL);
}

void deque_foreach_btof(struct deque *deque, void (*func)(void *data, void *user), void *user)
{
    deque_foreach(deque, func, user, ITER_DIR_TO_HEAD);
}

void deque_foreach_itob(struct deque *deque, size_t i, void (*func)(void *data, void *user), void *user)
{
    if (deque->size == 0) {
        return;
    }

    if (i == 0) {
        deque_foreach_ftob(deque, func, user);
        return;
    }

    size_t j = 0;
    struct deque_node *cursor = deque->head;
    do {
        cursor = cursor->next;
        j++;
    } while (cursor != NULL && j < i);

    while (cursor != NULL) {
        func(cursor->data, user);
        cursor = cursor->next;
    }
}

void deque_foreach_itof(struct deque *deque, size_t i, void (*func)(void *data, void *user), void *user)
{
    if (deque->size == 0 || i == 0) {
        return;
    }

    size_t j = 0;
    struct deque_node *cursor = deque->head;
    do {
        cursor = cursor->next;
        j++;
    } while (cursor != NULL && j < i);

    while (cursor != NULL) {
        func(cursor->data, user);
        cursor = cursor->prev;
    }
}

void deque_extend_b(struct deque *dst, struct deque *src)
{
    if (src->head == NULL) {
        return;
    }

    if (dst->tail == NULL) {
        dst->head = src->head;
        dst->tail = src->tail;
        dst->size = src->size;
        return;
    }

    dst->tail->next = src->head;
    src->head->prev = dst->tail;
    dst->size += src->size;
    dst->tail = src->tail;
}

void deque_extend_f(struct deque *dst, struct deque *src)
{
    if (src->tail == NULL) {
        return;
    }

    if (dst->head == NULL) {
        dst->head = src->head;
        dst->tail = src->tail;
        dst->size = src->size;
        return;
    }

    dst->head->prev = src->tail;
    src->tail->next = dst->head;
    dst->size += src->size;
    dst->head = src->head;
}
