// SPDX-License-Identifier: MIT

#pragma once

typedef int bool;

#define false 0
#define true  1
#define null  0

#define unused(__x) (void)(__x)

// Print a standard-format error message
#define err(...)                 errX(__VA_ARGS__, errF, errM)(__VA_ARGS__)
#define errX(__a, __b, __c, ...) __c
#define errM(__msg)              fprintf(stderr, PROGRAM_NAME ": %s\n", __msg)
#define errF(__fmt, ...)                               \
    do {                                               \
        char __buf[128];                               \
        snprintf(__buf, 128, __fmt, __VA_ARGS__);      \
        fprintf(stderr, PROGRAM_NAME ": %s\n", __buf); \
    } while (0)

// Kill the program with an error message and an optional pre-exit hook.
#define die(...)                      dieX(__VA_ARGS__, dieD, dieH, dieS)(__VA_ARGS__)
#define dieX(__a, __b, __c, __d, ...) __d

#define dieS(__msg)         \
    do {                    \
        errM(__msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

#define dieH(__msg, __hook) \
    do {                    \
        errM(__msg);        \
        __hook;             \
        exit(EXIT_FAILURE); \
    } while (0)

#define dieD(__msg, __hook, ...)  \
    do {                          \
        errF(__msg, __VA_ARGS__); \
        __hook;                   \
        exit(EXIT_FAILURE);       \
    } while (0)
