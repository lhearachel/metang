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

#include <stdio.h>
#include <stdlib.h>

#include "options.h"

int main(int argc, char **argv)
{
    options *opts = malloc(sizeof(*opts));
    int exit = EXIT_SUCCESS;
    if (argc == 1) {
        goto help;
    }

    if (!parseopts(&argc, &argv, opts)) {
        goto cleanup;
    }

    if (opts->result != OPTS_S) {
        char err[128];
        optserr(opts, err);
        fprintf(stderr, "metang: %s\n\n%s\n\n%s\n", err, short_usage, options_section);
        exit = EXIT_FAILURE;
        goto cleanup;
    }

    if (opts->help) {
    help:
        printf("%s\n\n%s\n\n%s\n", tag_line, short_usage, options_section);
        goto cleanup;
    }

    if (opts->version) {
        printf("%s\n", version);
        goto cleanup;
    }

    opts->to_stdout = opts->outfile == NULL;
    opts->fr_stdin = (argc == 0);

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

cleanup:
    free(opts);
    return exit;
}
