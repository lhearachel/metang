// SPDX-License-Identifier: MIT

#pragma once

#include <stdio.h>

#include "libs/strings.h"
#include "libs/vector.h"

typedef long bool;

#define BLOCK_SIZE 128

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
        (void)(__hook);     \
        exit(EXIT_FAILURE); \
    } while (0)

#define dieD(__msg, __hook, ...)  \
    do {                          \
        errF(__msg, __VA_ARGS__); \
        (void)(__hook);           \
        exit(EXIT_FAILURE);       \
    } while (0)

typedef enum sizesign {
    S_unbound,
    S_unsigned_8bit,
    S_unsigned_16bit,
    S_unsigned_32bit,
    S_unsigned_64bit,
    S_signed_8bit,
    S_signed_16bit,
    S_signed_32bit,
    S_signed_64bit,
} sizesign;

typedef struct args {
    bool        bitmask;  // --bitmask - defaults to "false"
    const char *lang;     // --lang    - defaults to "c"
    const char *inguard;  // --guard   - defaults to "METANG"
    const char *intag;    // --tag     - defaults to basename of the input file
    const char *insized;  // --sized   - defaults to 0 (no bound or sign)
    const char *outfname; // --output  - defaults to stdout
    const char *infname;  // <file>    - specify "-" to use stdin

    FILE    *infile;     // The actual input stream
    string   tag;        // Processed copy of the input tag
    string   guard;      // Processed copy of the input guard
    sizesign sizesign;   // Interpreted size and sign bindings for the enum
    string   infbase;    // Basename of the input file
    string   outfbaseup; // Uppercased version of the output file's basename
} args;

typedef struct seqelem {
    string symbol;
    long   value;
} seqelem;

typedef struct sequence {
    vector elems; // T = seqelem
    long   maxsymlen;
} sequence;
