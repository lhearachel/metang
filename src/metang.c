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
#include "metang.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "deque.h"
#include "generate.h"
#include "strlib.h"
#include "version.h"

// clang-format off
static const char *version = METANG_VERSION;

static const char *tag_line = "metang - Generate multi-purpose C headers for enumerators";

static const char *short_usage = "Usage: metang [options] [<input_file>]";

static const char *options = ""
    "Options:\n"
    "  -a, --append <entry>         Append <entry> to the input listing.\n"
    "  -p, --prepend <entry>        Prepend <entry> to the input listing.\n"
    "  -n, --start-from <number>    Start enumeration from <number>.\n"
    "  -o, --output <file>          Write output to <file>.\n"
    "  -l, --leader <leader>        Use <leader> as a prefix for generated symbols.\n"
    "  -t, --tag-name <name>        Use <name> as the base tag for enums and lookup\n"
    "                               tables.\n"
    "  -c, --tag-case <case>        Customize the casing of generated tags for enums\n"
    "                               and lookup tables. Options: snake, pascal\n"
    "  -G, --preproc-guard <guard>  Use <guard> as a prefix for conditional\n"
    "                               preprocessor directives.\n"
    "  -B, --bitmask                If specified, generate symbols for a bitmask.\n"
    "                               This option is incompatible with the “-D” and\n"
    "                               “-n” options.\n"
    "  -D, --allow-overrides        If specified, allow direct value-assignment.\n"
    "  -h, --help                   Display this help text and exit.\n"
    "  -v, --version                Display the program version number and exit."
    "";
// clang-format on

static void exit_if(bool cond, int (*exit_func)(const char *fmt, va_list args), const char *fmt, ...);
static int exit_info(const char *fmt, va_list args);
static int exit_fail(const char *fmt, va_list args);

static bool match_opt(const char *opt, const char *shortopt, const char *longopt);
static void parse_options(int *argc, const char ***argv, struct options *opts);

static ssize_t read_line(char **lineptr, size_t *n, FILE *stream);
static bool read_from_stream(FILE *stream, struct deque *deque, const bool allow_overrides);
static bool read_from_file(const char *fname, struct deque *deque, const bool allow_overrides);

static struct enumerator *enumerator_new(const char *lvalue, long rvalue, bool direct);
static void enumerator_free(void *data);

#ifndef NDEBUG
static void printf_deque_node(void *data, void *user);
static void printf_deque_enum_node(void *data, void *user);
#endif // NDEBUG

int main(int argc, const char **argv)
{
    int orig_argc = argc;
    const char **orig_argv = argv;

    // If no args given and no file is piped to `stdin`, then act like `-h`.
    exit_if(argc < 2 && isatty(STDIN_FILENO), exit_info, "%s\n\n%s\n\n%s\n", tag_line, short_usage, options);

    argv++;
    argc--;

    struct options options = {
        .append = deque_new(),
        .prepend = deque_new(),
        .start_from = 0,
        .leader = NULL,
        .preproc_guard = "METANG",
        .output_file = NULL,
        .input_file = NULL,
        .tag = NULL,
        .tag_case = TAG_SNAKE_CASE,
        .bitmask = false,
        .allow_overrides = false,
        .to_stdout = false,
        .from_stdin = false,
    };

    exit_if(options.append == NULL || options.prepend == NULL, exit_fail, "metang: failure ahead of option parsing: “%s”\n", strerror(errno));
    parse_options(&argc, &argv, &options);
    exit_if(argc > 1, exit_fail, "metang: unexpected positional arguments\n%s\n", short_usage);

    options.to_stdout = options.output_file == NULL;
    options.output_file = options.to_stdout ? "stdout" : options.output_file;
    options.from_stdin = (argc == 0);
    options.input_file = options.from_stdin ? "stdin" : *argv;

#ifndef NDEBUG
    printf("--- METANG OPTIONS ---\n");
    printf("append:\n");
    deque_foreach_ftob(options.append, printf_deque_node, NULL);
    printf("prepend:\n");
    deque_foreach_ftob(options.prepend, printf_deque_node, NULL);
    printf("start from:     “%ld”\n", options.start_from);
    printf("leader:         “%s”\n", options.leader ? options.leader : "default");
    printf("tag:            “%s”\n", options.tag ? options.tag : "default");
    printf("tag case:       “%s”\n", options.tag_case == TAG_SNAKE_CASE ? "snake" : "pascal");
    printf("preproc guard:  “%s”\n", options.preproc_guard);
    printf("allow override? “%s”\n", options.allow_overrides ? "yes" : "no");
    printf("output file:    “%s”\n", options.output_file);
    printf("input file:     “%s”\n", options.input_file);
#endif

    struct deque *input_lines = deque_new();
    exit_if(input_lines == NULL, exit_fail, "metang: failure ahead of reading input: “%s”\n", strerror(errno));

    bool input_good = options.from_stdin
        ? read_from_stream(stdin, input_lines, options.allow_overrides)
        : read_from_file(*argv, input_lines, options.allow_overrides);
    exit_if(!input_good, exit_fail, "metang: failure while reading input: “%s”\n", strerror(errno));

#ifndef NDEBUG
    printf("\n--- METANG INPUT ---\n");
    printf("lines:\n");
    deque_foreach_ftob(input_lines, printf_deque_enum_node, NULL);
#endif

    struct deque *all_input = input_lines;
    deque_extend_f(all_input, options.prepend);
    deque_extend_b(all_input, options.append);

#ifndef NDEBUG
    printf("\n--- METANG OUTPUT ---\n");
#endif

    const char *output = generate(all_input, &options, orig_argc, orig_argv);
    FILE *fout = stdout;
    if (!options.to_stdout) {
        fout = fopen(options.output_file, "w");
        exit_if(fout == NULL, exit_fail, "metang: failure while writing output: “%s”\n", strerror(errno));
    }

    fputs(output, fout);
    fflush(fout);
    if (!options.to_stdout) {
        fclose(fout);
    }

    free((char *)output);

    deque_free(all_input, enumerator_free);
    free(options.prepend);
    free(options.append);

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
    // This can happen if the user pipes input through `stdin` and provides no
    // program options.
    if (*argc == 0) {
        return;
    }

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
        if (match_opt(opt, "-B", "--bitmask")) {
            opts->bitmask = true;
            continue;
        }

        if (match_opt(opt, "-D", "--allow-overrides")) {
            opts->allow_overrides = true;
            continue;
        }

        // Options with an argument
        exit_if(*argc < 1, exit_fail, "metang: missing argument for option “%s”\n", opt);

        const char *arg = (*argv)[0];
        if (match_opt(opt, "-a", "--append")) {
            struct enumerator *app = enumerator_new(usnake(arg, NULL), 0, false);
            exit_if(!deque_push_b(opts->append, app), exit_fail, "metang: could not add to append options: %s\n", strerror(errno));
        } else if (match_opt(opt, "-p", "--prepend")) {
            struct enumerator *pre = enumerator_new(usnake(arg, NULL), 0, false);
            exit_if(!deque_push_b(opts->prepend, pre), exit_fail, "metang: could not add to prepend options: %s\n", strerror(errno));
        } else if (match_opt(opt, "-n", "--start-from")) {
            opts->start_from = strtol(arg, NULL, 10);
            exit_if(errno, exit_fail, "metang: could not convert start-from option: %s\n", strerror(errno));
        } else if (match_opt(opt, "-o", "--output")) {
            opts->output_file = arg;
        } else if (match_opt(opt, "-l", "--leader")) {
            opts->leader = arg;
        } else if (match_opt(opt, "-c", "--tag-case")) {
            if (strcmp(arg, "pascal") == 0) {
                opts->tag_case = TAG_PASCAL_CASE;
            } else if (strcmp(arg, "snake") == 0) {
                opts->tag_case = TAG_SNAKE_CASE;
            } else {
                exit_if(true, exit_fail, "metang: unrecognized argument “%s” for option “%s”\n", arg, opt);
            }
        } else if (match_opt(opt, "-t", "--tag-name")) {
            opts->tag = arg;
        } else if (match_opt(opt, "-G", "--preproc-guard")) {
            opts->preproc_guard = arg;
        }

        (*argv)++;
        (*argc)--;
    } while (*argc > 0);

    exit_if(opts->bitmask && (opts->allow_overrides || opts->start_from),
            exit_fail, "metang: invalid option state; cannot override values for bitmasks\n%s\n\n%s", short_usage, options);
}

static ssize_t read_line(char **lineptr, size_t *n, FILE *stream)
{
    if (lineptr == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    int c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        if (*lineptr == NULL) {
            return -1;
        }

        *n = 128;
    }

    size_t pos = 0;
    do {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }

            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }

            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos++] = c;
        if (c == '\n') {
            break;
        }
    } while ((c = fgetc(stream)) != EOF);

    (*lineptr)[pos] = '\0';
    return pos;
}

static bool read_from_stream(FILE *stream, struct deque *input_lines, const bool allow_overrides)
{
    char *buf = NULL;
    size_t buf_size = 0;
    ssize_t read_size;

    while ((read_size = read_line(&buf, &buf_size, stream)) != -1) {
        char *line = calloc(read_size, sizeof(char));
        strncpy(line, buf, read_size - 1); // trim the newline character

        struct enumerator *mem;
        if (!allow_overrides) {
            exit_if(strchr(line, '='), exit_fail, "metang: input contains unpermitted override; did you forget “-D”?\n");
            mem = enumerator_new(usnake(line, NULL), 0, false);
        } else if (strchr(line, '=')) {
            char *token = strtok(line, "=");
            char *token_b = token + strlen(token) - 1;
            if (isspace(*token_b)) {
                *token_b = '\0';
            }

            char *rvalue = token_b + 2;
            mem = enumerator_new(usnake(token, NULL), strtol(rvalue, NULL, 10), true);
        } else {
            mem = enumerator_new(usnake(line, NULL), 0, false);
        }

        free(line);
        exit_if(mem == NULL, exit_fail, "metang: memory allocation failure while reading input: “%s”\n", strerror(errno));
        deque_push_b(input_lines, mem);
    }

    free(buf);
    return true;
}

static bool read_from_file(const char *fpath, struct deque *input_lines, const bool allow_overrides)
{
    FILE *f = fopen(fpath, "r");
    if (f == NULL) {
        return false;
    }

    bool result = read_from_stream(f, input_lines, allow_overrides);

    fclose(f);
    return result;
}

static struct enumerator *enumerator_new(const char *lvalue, long rvalue, bool direct)
{
    struct enumerator *mem = calloc(1, sizeof(struct enumerator));
    if (mem == NULL) {
        return NULL;
    }

    mem->lvalue = lvalue;
    mem->rvalue = rvalue;
    mem->direct = direct;

    return mem;
}

static void enumerator_free(void *data)
{
    struct enumerator *mem = data;
    free((char *)mem->lvalue);
    free(mem);
}

static bool match_opt(const char *opt, const char *shortopt, const char *longopt)
{
    return (shortopt != NULL && strcmp(opt, shortopt) == 0)
        || (longopt != NULL && strcmp(opt, longopt) == 0);
}

#ifndef NDEBUG
static void printf_deque_node(void *data, void *user)
{
    (void)user;
    char *s = data;
    printf("  - “%s”\n", s);
}

static void printf_deque_enum_node(void *data, void *user)
{
    (void)user;
    struct enumerator *mem = data;
    if (mem->direct) {
        printf("  - “%s” = %ld\n", mem->lvalue, mem->rvalue);
    } else {
        printf("  - “%s”\n", mem->lvalue);
    }
}
#endif // NDEBUG
