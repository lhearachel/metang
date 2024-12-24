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

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deque.h"

typedef intptr_t ssize_t;

// clang-format off
static const char *version = "0.1.0";

static const char *tag_line = "metang - meta-program for C constants generation";

static const char *short_usage = "Usage: metang [options] [<input_file>]";

static const char *options = ""
    "Options:\n"
    "  -a, --append <entry>         Append <entry> to the input listing.\n"
    "  -p, --prepend <entry>        Prepend <entry> to the input listing.\n"
    "  -n, --start-from <number>    Start enumeration from <number>.\n"
    "  -o, --output <file>          Write output to <file>.\n"
    "  -D, --allow-override         If specified, allow direct value-assignment.\n"
    "  -h, --help                   Display this help text and exit.\n"
    "  -v, --version                Display the program version number and exit."
    "";
// clang-format on

struct options {
    struct deque *append;
    struct deque *prepend;
    ssize_t start_from;
    bool allow_override;
    const char *preproc_guard;
    const char *output_file;
};

static void exit_if(bool cond, int (*exit_func)(const char *fmt, va_list args), const char *fmt, ...);
static int exit_info(const char *fmt, va_list args);
static int exit_fail(const char *fmt, va_list args);

static bool match_opt(const char *opt, const char *shortopt, const char *longopt);
static void parse_options(int *argc, const char ***argv, struct options *opts);
static void printf_deque_node(void *data);

int main(int argc, const char **argv)
{
    exit_if(argc < 2, exit_info, "%s\n\n%s\n\n%s\n", tag_line, short_usage, options);

    argv++;
    argc--;

    struct options options = {
        .append = deque_new(),
        .prepend = deque_new(),
        .start_from = 0,
        .allow_override = false,
        .output_file = NULL,
    };

    exit_if(options.append == NULL || options.prepend == NULL, exit_fail, "metang: failure ahead of option parsing: “%s”\n", strerror(errno));
    parse_options(&argc, &argv, &options);
    exit_if(argc > 1, exit_fail, "metang: unexpected positional arguments\n%s\n", short_usage);

    bool from_stdin = (argc == 0);

#ifndef NDEBUG
    printf("--- METANG OPTIONS ---\n");
    printf("append:\n");
    deque_foreach_ftob(options.append, printf_deque_node);
    printf("prepend:\n");
    deque_foreach_ftob(options.prepend, printf_deque_node);
    printf("start from:     “%ld”\n", options.start_from);
    printf("allow override? “%s”\n", options.allow_override ? "yes" : "no");
    printf("output file:    “%s”\n", options.output_file == NULL ? "stdout" : options.output_file);
    printf("input file:     “%s”\n", from_stdin ? "stdin" : *argv);
#endif

    struct deque *input_lines = deque_new();
    exit_if(input_lines == NULL, exit_fail, "metang: failure ahead of reading input: “%s”\n", strerror(errno));

    return EXIT_SUCCESS;
}

static void exit_if(bool cond, int (*exit_func)(const char *fmt, va_list args), const char *fmt, ...)
{
    if (cond) {
        va_list args;
        va_start(args, fmt);
        int exit_code = exit_func(fmt, args);
        va_end(args);
        exit(exit_code);
    }
}

static int exit_info(const char *fmt, va_list args)
{
    vfprintf(stdout, fmt, args);
    return EXIT_SUCCESS;
}

static int exit_fail(const char *fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    return EXIT_FAILURE;
}

static void parse_options(int *argc, const char ***argv, struct options *opts)
{
    do {
        const char *opt = (*argv)[0];
        if (opt[0] != '-' || (opt[1] == '-' && opt[2] == '\0')) {
            break;
        }

        (*argv)++;
        (*argc)--;

        exit_if(match_opt(opt, "-h", "--help"), exit_info, "%s\n\n%s\n\n%s\n", tag_line, short_usage, options);
        exit_if(match_opt(opt, "-v", "--version"), exit_info, "%s\n", version);

        // Options with no argument
        if (match_opt(opt, "-D", "--allow-override")) {
            opts->allow_override = true;
            continue;
        }

        // Options with an argument
        exit_if(*argc < 1, exit_fail, "metang: missing argument for option “%s”\n", opt);

        const char *arg = (*argv)[0];
        if (match_opt(opt, "-a", "--append")) {
            exit_if(!deque_push_b(opts->append, (void *)arg), exit_fail, "metang: could not add to append options: %s\n", strerror(errno));
        } else if (match_opt(opt, "-p", "--prepend")) {
            exit_if(!deque_push_b(opts->prepend, (void *)arg), exit_fail, "metang: could not add to prepend options: %s\n", strerror(errno));
        } else if (match_opt(opt, "-n", "--start-from")) {
            opts->start_from = strtol(arg, NULL, 10);
            exit_if(errno, exit_fail, "metang: could not convert start-from option: %s\n", strerror(errno));
        } else if (match_opt(opt, "-o", "--output")) {
            opts->output_file = arg;
        }

        (*argv)++;
        (*argc)--;
    } while (*argc > 0);
}

static bool match_opt(const char *opt, const char *shortopt, const char *longopt)
{
    return (shortopt != NULL && strcmp(opt, shortopt) == 0)
        || (longopt != NULL && strcmp(opt, longopt) == 0);
}

static void printf_deque_node(void *data)
{
    char *s = data;
    printf("  - “%s”\n", s);
}
