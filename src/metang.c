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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "strbuf.h"

static int pargv(int *argc, char ***argv, options *opts);
static str fload(FILE *f);
static strlist *readlines(FILE *f);

int main(int argc, char **argv)
{
    int exit;
    options *opts = malloc(sizeof(*opts));
    if ((exit = pargv(&argc, &argv, opts))) {
        exit--;
        goto cleanup;
    }

#ifndef NDEBUG
    printf("--- METANG OPTIONS ---\n");
    printf("append:\n");
    for (usize i = 0; i < opts->append_count; i++) {
        printf("  - %s\n", opts->append[i]);
    }
    printf("prepend:\n");
    for (usize i = 0; i < opts->prepend_count; i++) {
        printf("  - %s\n", opts->prepend[i]);
    }
    printf("start from:   “%ld”\n", opts->start);
    printf("leader:       “%s”\n", opts->leader);
    printf("tag:          “%s”\n", opts->tag);
    printf("guard:        “%s”\n", opts->guard);
    printf("outfile:      “%s”\n", opts->outfile);
    printf("infile:       “%s”\n", opts->infile);
    printf("casing:       “%s”\n", opts->casing == TAG_SNAKE_CASE ? "snake" : "pascal");
    printf("bitmask?      “%s”\n", opts->bitmask ? "yes" : "no");
    printf("overrides?    “%s”\n", opts->overrides ? "yes" : "no");
    printf("stdout?       “%s”\n", opts->to_stdout ? "yes" : "no");
    printf("stdin?        “%s”\n", opts->fr_stdin ? "yes" : "no");
#endif // NDEBUG

    FILE *fin = opts->fr_stdin ? stdin : fopen(opts->infile, "rb");
    if (fin == NULL) {
        fprintf(stderr,
                "metang: could not open input file “%s”: %s",
                opts->infile, strerror(errno));
        goto cleanup;
    }

    strlist *input = readlines(fin);
    strlist *line, *next;

#ifndef NDEBUG
    printf("--- METANG INPUT ---\n");

    line = input;
    while (line) {
        printf("%.*s\n", (int)line->elem.len, line->elem.buf);
        line = line->next;
    }
#endif // NDEBUG

    free(input->elem.buf);

    line = input;
    while (line) {
        next = line->next;
        free(line);
        line = next;
    }

cleanup:
    free(opts);
    return exit;
}

#define PARGV_EXIT_SUCCESS EXIT_SUCCESS + 1
#define PARGV_EXIT_FAILURE EXIT_FAILURE + 1

static int pargv(int *argc, char ***argv, options *opts)
{
    if (*argc == 1 && isatty(STDIN_FILENO)) {
        goto help;
        return PARGV_EXIT_SUCCESS;
    }

    (*argc)--;
    (*argv)++;

    if (!parseopts(argc, argv, opts)) {
        char err[128];
        optserr(opts, err);
        fprintf(stderr,
                "metang: %s\n\n%s\n\n%s\n",
                err, short_usage, options_section);
        return PARGV_EXIT_FAILURE;
    }

    if (opts->help) {
    help:
        printf("%s\n\n%s\n\n%s\n", tag_line, short_usage, options_section);
        return PARGV_EXIT_SUCCESS;
    }

    if (opts->version) {
        printf("%s\n", version);
        return PARGV_EXIT_SUCCESS;
    }

    if (opts->bitmask && (opts->overrides || opts->start)) {
        fprintf(stderr,
                "metang: invalid option state; cannot override values for bitmasks\n%s\n\n%s\n",
                short_usage, options_section);
        return PARGV_EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static str fload(FILE *f)
{
    strbuf sbuf = {0};
    sbuf.cap = 1 << 16;
    sbuf.s.buf = malloc(sbuf.cap);
    if (sbuf.s.buf == NULL) {
        fprintf(stderr, "metang: allocation failure for reading input\n");
        fclose(f);
        return sbuf.s;
    }

    usize read;
    char buf[256];
    while ((read = fread(buf, 1, 1 << 15, f)) != 0) {
        str t = str(buf, strlen(buf));
        if (!bufextend(&sbuf, t)) {
            fprintf(stderr, "metang: WARNING! allocation failure while reading input\n");
            break;
        }
    }

    return sbuf.s;
}

static strlist *readlines(FILE *f)
{
    strpair pair = {0};
    pair.tail = fload(f);

    strlist *head = NULL;
    strlist **tail = &head;
    while (pair.tail.len) {
        pair = strcut(pair.tail, '\n');
        *tail = malloc(sizeof(**tail));
        (*tail)->next = NULL;
        (*tail)->elem = pair.head;
        tail = &(*tail)->next;
    }

    fclose(f);
    return head;
}
