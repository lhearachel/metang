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

// Included for LSP convenience. Turn this flag ON to optimize structures
// into a format that allows generalizing push, pop, and iteration.
// #define DEQUE_NDEBUG

enum deque_iter_dir {
    ITER_DIR_TO_HEAD = 0,
    ITER_DIR_TO_TAIL,
    ITER_DIR_MAX,
};

struct deque_node {
    void *data;

#ifdef DEQUE_NDEBUG
    struct deque_node *neighbors[ITER_DIR_MAX];
#else
    struct deque_node *prev;
    struct deque_node *next;
#endif
};

struct deque {
    size_t size;

#ifdef DEQUE_NDEBUG
    struct deque_node *ptrs[ITER_DIR_MAX];
#else
    struct deque_node *head;
    struct deque_node *tail;
#endif
};

#ifdef DEQUE_NDEBUG
#define prev neighbors[ITER_DIR_TO_HEAD]
#define next neighbors[ITER_DIR_TO_TAIL]
#define head ptrs[ITER_DIR_TO_HEAD]
#define tail ptrs[ITER_DIR_TO_TAIL]
#endif // DEQUE_NDEBUG

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

#ifdef DEQUE_NDEBUG
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
#endif // DEQUE_NDEBUG

struct deque_node *deque_push_b(struct deque *deque, void *data)
{
#ifdef DEQUE_NDEBUG
    return deque_push(deque, data, ITER_DIR_TO_TAIL);
#else
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

    node->prev = deque->tail; // set new node's prev/next to tail/head, respectively
    deque->tail->next = node; // set the neighbor of the head/tail to the new node
    deque->tail = node;       // set the head/tail of the deque to the new node
    deque->size++;
    return node;
#endif // DEQUE_NDEBUG
}

struct deque_node *deque_push_f(struct deque *deque, void *data)
{
#ifdef DEQUE_NDEBUG
    return deque_push(deque, data, ITER_DIR_TO_HEAD);
#else
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

    node->next = deque->head; // set new node's prev/next to tail/head, respectively
    deque->head->prev = node; // set the neighbor of the head/tail to the new node
    deque->head = node;       // set the head/tail of the deque to the new node
    deque->size++;
    return node;
#endif // DEQUE_NDEBUG
}

#ifdef DEQUE_NDEBUG
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
#endif // DEQUE_NDEBUG

struct deque_node *deque_pop_b(struct deque *deque)
{
#ifdef DEQUE_NDEBUG
    return deque_pop(deque, ITER_DIR_TO_TAIL);
#else
    if (deque->size == 0) {
        return NULL;
    }

    struct deque_node *pop = deque->tail; // grab the head/tail
    deque->tail = pop->prev;              // set the head/tail to popped node's next/prev, respectively
    pop->prev = NULL;                     // zero out the popped node's next/prev
    return pop;
#endif // DEQUE_NDEBUG
}

struct deque_node *deque_pop_f(struct deque *deque)
{
#ifdef DEQUE_NDEBUG
    return deque_pop(deque, ITER_DIR_TO_HEAD);
#else
    if (deque->size == 0) {
        return NULL;
    }

    struct deque_node *pop = deque->head; // grab the head/tail
    deque->head = pop->next;              // set the head/tail to popped node's next/prev, respectively
    pop->next = NULL;                     // zero out the popped node's next/prev
    return pop;
#endif // DEQUE_NDEBUG
}

void *deque_peek_b(struct deque *deque)
{
    return deque->tail->data;
}

void *deque_peek_f(struct deque *deque)
{
    return deque->head->data;
}

#ifdef DEQUE_NDEBUG
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
#endif // DEQUE_NDEBUG

void deque_foreach_ftob(struct deque *deque, void (*func)(void *data, void *user), void *user)
{
#ifdef DEQUE_NDEBUG
    deque_foreach(deque, func, user, ITER_DIR_TO_TAIL);
#else
    if (deque->size == 0) {
        return;
    }

    struct deque_node *cursor = deque->head;
    do {
        func(cursor->data, user);
        cursor = cursor->next;
    } while (cursor != NULL);
#endif // DEQUE_NDEBUG
}

void deque_foreach_btof(struct deque *deque, void (*func)(void *data, void *user), void *user)
{
#ifdef DEQUE_NDEBUG
    deque_foreach(deque, func, user, ITER_DIR_TO_HEAD);
#else
    if (deque->size == 0) {
        return;
    }

    struct deque_node *cursor = deque->tail;
    do {
        func(cursor->data, user);
        cursor = cursor->prev;
    } while (cursor != NULL);
#endif // DEQUE_NDEBUG
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
