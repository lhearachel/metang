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
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "alloc.h"
#include "options.h"
#include "strbuf.h"

static int pargv(int *argc, char ***argv, options *opts);
static str fload(FILE *f);
static strlist *readlines(FILE *f);

extern const str version;
extern const str tag_line;
extern const str short_usage;
extern const str options_section;

arena *global;

int main(int argc, char **argv)
{
    arena a = arena_new(1 << 16);
    global = &a;

    FILE *fin = NULL, *fout = NULL;
    options *opts = malloc(sizeof(*opts));

    int exit = pargv(&argc, &argv, opts);
    if (exit) {
        exit--;
        goto cleanup;
    }

#ifndef NDEBUG
    printf("--- METANG OPTIONS ---\n");
    printf("append:\n");
    for (usize i = 0; i < opts->append_count; i++) {
        printf("  - %s\n", opts->append[i].buf);
    }
    printf("prepend:\n");
    for (usize i = 0; i < opts->prepend_count; i++) {
        printf("  - %s\n", opts->prepend[i].buf);
    }
    printf("start from:   “%ld”\n", opts->start);
    printf("leader:       “%s”\n", opts->leader.buf);
    printf("tag:          “%s”\n", opts->tag.buf);
    printf("guard:        “%s”\n", opts->guard.buf);
    printf("casing:       “%s”\n", opts->casing == TAG_SNAKE_CASE ? "snake" : "pascal");
    printf("bitmask?      “%s”\n", opts->bitmask ? "yes" : "no");
    printf("overrides?    “%s”\n", opts->overrides ? "yes" : "no");
    printf("outfile:      “%s”\n", opts->outfile.len == 0 ? "stdout" : opts->outfile.buf);
    printf("infile:       “%s”\n", opts->outfile.len == 0 ? "stdin" : opts->infile.buf);
#endif // NDEBUG

    fin = opts->infile.len == 0 ? stdin : fopen(opts->infile.buf, "rb");
    if (fin == NULL) {
        fprintf(stderr,
                "metang: could not open input file “%s”: %s",
                opts->infile.buf, strerror(errno));
        goto cleanup;
    }

    fout = opts->outfile.len == 0 ? stdout : fopen(opts->outfile.buf, "wb");
    if (fout == NULL) {
        fprintf(stderr,
                "metang: could not open output file “%s”: %s",
                opts->outfile.buf, strerror(errno));
        goto cleanup;
    }

    if (setjmp(global->env)) {
        fprintf(stderr, "metang: memory allocation failure");
        goto cleanup;
    }

    strlist *input = readlines(fin);

#ifndef NDEBUG
    printf("--- METANG INPUT ---\n");

    strlist *line = input;
    while (line) {
        printf("%.*s\n", (int)line->elem.len, line->elem.buf);
        line = line->next;
    }

    printf("--- METANG OUTPUT ---\n");
#endif // NDEBUG

cleanup:
    fin ? fclose(fin) : 0;
    fout ? fclose(fout) : 0;
    free(global->mem);
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
                err, short_usage.buf, options_section.buf);
        return PARGV_EXIT_FAILURE;
    }

    if (opts->help) {
    help:
        printf("%s\n\n%s\n\n%s\n", tag_line.buf, short_usage.buf, options_section.buf);
        return PARGV_EXIT_SUCCESS;
    }

    if (opts->version) {
        printf("%s\n", version.buf);
        return PARGV_EXIT_SUCCESS;
    }

    if (opts->bitmask && (opts->overrides || opts->start)) {
        fprintf(stderr,
                "metang: invalid option state; cannot override values for bitmasks\n%s\n\n%s\n",
                short_usage.buf, options_section.buf);
        return PARGV_EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static str fload(FILE *f)
{
    usize read;
    char buf[1 << 15];

    // The arena's internal memory can change during successive claims, so we
    // must track the expected starting position.
    usize bufofs = nextofs(global, alignof(char));
    usize buflen = 0;
    while ((read = fread(buf, 1, 1 << 15, f)) != 0) {
        claim(global, buf, strlen(buf), A_F_EXTEND);
        buflen += read;
    }

    str s = strnew(global->mem + bufofs, buflen);
    return s;
}

static strlist *readlines(FILE *f)
{
    strpair pair = {0};
    strpair line = {0};
    line.tail = fload(f);

    strlist *head = NULL;
    strlist **tail = &head;
    while (line.tail.len) {
        line = strcut(&line.tail, '\n');
        pair = strcut(&line.head, '#');
        pair.head.len = strtrim(&pair.head);

        *tail = new (global, strlist, 1, A_F_EXTEND);
        (*tail)->next = NULL;
        (*tail)->elem = pair.head;

        tail = &(*tail)->next;
    }

    fclose(f);
    return head;
}
