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
#include "generator.h"
#include "options.h"
#include "strbuf.h"

static int pargv(int *argc, char ***argv, options *opts);
static str fload(FILE *f);
static enumerator *enumerate(FILE *f, options *opts);

extern const str version;
extern const str tag_line;
extern const str short_usage;
extern const str commands_section;
extern const str global_options_section;
extern const str enum_options_section;
extern const str mask_notes_section;

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
    printf("mode:         “%s”\n", (opts->mode & OPTS_M_ENUM) ? "enum" : "mask");

    if (opts->mode & OPTS_M_ENUM) {
        printf("append:");
        if (opts->append_count > 0) {
            printf("\n");
            for (usize i = 0; i < opts->append_count; i++) {
                printf("  - %s\n", opts->append[i].buf);
            }
        } else {
            printf("       []\n");
        }
        printf("prepend:");
        if (opts->prepend_count > 0) {
            for (usize i = 0; i < opts->prepend_count; i++) {
                printf("  - %s\n", opts->prepend[i].buf);
            }
        } else {
            printf("      []\n");
        }
        printf("start from:   “%ld”\n", opts->start);
        printf("overrides?    “%s”\n", opts->overrides ? "yes" : "no");
    }

    printf("leader:       “%s”\n", opts->leader.buf);
    printf("tag:          “%s”\n", opts->tag.buf);
    printf("guard:        “%s”\n", opts->guard.buf);
    printf("casing:       “%s”\n", opts->casing == TAG_SNAKE_CASE ? "snake" : "pascal");
    printf("lang:         “%s”\n", opts->lang.buf);
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
        goto cleanup;
    }

    opts->outfile = opts->outfile.len == 0 ? strnew("stdout") : opts->outfile;
    enumerator *input = enumerate(fin, opts);

#ifndef NDEBUG
    printf("--- METANG INPUT ---\n");

    enumerator *line = input;
    while (line) {
        printf("%.*s = %ld\n", (int)line->ident.len, line->ident.buf, line->assignment);
        line = line->next;
    }

    printf("--- METANG OUTPUT ---\n");
#endif // NDEBUG

    generators[opts->genf].genfunc(input, opts, fout);

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
    }

    (*argc)--;
    (*argv)++;

    str mode = strnew(**argv, strlen(**argv));
    if (streq(&mode, &strnew("enum"))) {
        opts->mode = OPTS_M_ENUM;
    } else if (streq(&mode, &strnew("mask"))) {
        opts->mode = OPTS_M_MASK;
    } else if (streq(&mode, &strnew("help"))) {
    help:
        printf("%s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s\n",
               tag_line.buf,
               short_usage.buf,
               commands_section.buf,
               global_options_section.buf,
               enum_options_section.buf,
               mask_notes_section.buf);
        return PARGV_EXIT_SUCCESS;
    } else if (streq(&mode, &strnew("version"))) {
        printf("%s\n", version.buf);
        return PARGV_EXIT_SUCCESS;
    } else {
        fprintf(stderr,
                "metang: Unexpected value for COMMAND: “%s”\n",
                mode.buf);
        fprintf(stderr, "%s\n\n%s\n", short_usage.buf, commands_section.buf);
        return PARGV_EXIT_FAILURE;
    }

    (*argc)--;
    (*argv)++;

    if (!parseopts(argc, argv, opts)) {
        char buf[128];
        str errbuf = strnew(buf);
        optserr(opts, &errbuf);
        fprintf(stderr,
                "metang: %.*s\n\n%s\n\n%s\n\n%s\n\n%s\n\n%s\n",
                (int)errbuf.len, errbuf.buf,
                short_usage.buf,
                commands_section.buf,
                global_options_section.buf,
                enum_options_section.buf,
                mask_notes_section.buf);
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

static inline usize max_of(usize a, usize b)
{
    return a > b ? a : b;
}

static inline usize assign_strlen(isize i)
{
    int r = 1 + (i < 0);
    usize n = (i < 0) ? -i : i;
    while (n > 9) {
        n /= 10;
        r++;
    }

    return r;
}

static enumerator *enumerate(FILE *f, options *opts)
{
    strpair pair = {0};
    strpair line = {0};
    line.tail = fload(f);

    enumerator *head = NULL;
    enumerator **tail = &head;
    isize val = opts->start;
    for (usize i = 0; i < opts->prepend_count; i++) {
        *tail = new (global, enumerator, 1, A_F_ZERO | A_F_EXTEND);
        (*tail)->next = NULL;
        (*tail)->ident = opts->prepend[i];
        (*tail)->assignment = val;
        val++;

        head->max_ident_len = max_of((*tail)->ident.len, head->max_ident_len);
        head->max_assign_len = max_of(assign_strlen((*tail)->assignment), head->max_assign_len);
        head->count++;

        tail = &(*tail)->next;
    }

    while (line.tail.len) {
        line = strcut(&line.tail, '\n');
        pair = strcut(&line.head, '#');
        pair = strcut(&pair.head, '=');
        pair.head.len = strtrim(&pair.head);
        if (pair.tail.len > 0) {
            if (opts->mode == OPTS_M_MASK) {
                fprintf(stderr,
                        "metang: Per-value assignments are not permitted for bitmasks\n");
                longjmp(global->env, 1);
            } else if (!strtolong(&pair.tail, &val)) {
                fprintf(stderr,
                        "metang: Expected numeric value for assignment, but found “%s”\n",
                        pair.tail.buf);
                longjmp(global->env, 1);
            }
        }

        *tail = new (global, enumerator, 1, A_F_ZERO | A_F_EXTEND);
        (*tail)->next = NULL;
        (*tail)->ident = pair.head;
        (*tail)->assignment = val;
        val++;

        head->max_ident_len = max_of((*tail)->ident.len, head->max_ident_len);
        head->max_assign_len = max_of(assign_strlen((*tail)->assignment), head->max_assign_len);
        head->count++;

        tail = &(*tail)->next;
    }

    for (usize i = 0; i < opts->append_count; i++) {
        *tail = new (global, enumerator, 1, A_F_ZERO | A_F_EXTEND);
        (*tail)->next = NULL;
        (*tail)->ident = opts->append[i];
        (*tail)->assignment = val;
        val++;

        head->max_ident_len = max_of((*tail)->ident.len, head->max_ident_len);
        head->max_assign_len = max_of(assign_strlen((*tail)->assignment), head->max_assign_len);
        head->count++;

        tail = &(*tail)->next;
    }

    return head;
}
