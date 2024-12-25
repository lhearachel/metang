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

#include "generate.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deque.h"
#include "metang.h"
#include "strlib.h"

static char *write_header(char **output, const char *incg, struct options *opts, const int argc, const char **argv);
static char *write_enum(char **output, struct deque *input, const char *lead, const char *enum_t, struct options *opts);
static char *write_defs(char **output, struct deque *input, const char *lead, struct options *opts);
static char *write_lookup(char **output, struct deque *input, const char *lead, const char *enum_t, struct options *opts);
static char *write_footer(char **output, const char *incg);

struct entry_user {
    char **bufp;
    ssize_t it;
    const char *lead;
    const char *fmt;
};

static void write_member_entry(void *data, void *user_v);
static void write_lookup_entry(void *data, void *user_v);

// clang-format off
static const char *header_p1_fmt = ""
    "/*\n"
    " * This file was generated by metang; DO NOT MODIFY IT!!\n"
    " * Source file: %s\n"
    " * Program options:"
    "";

static const char *header_p2_fmt = ""
    "\n" // trails from end of `Program args`
    " */\n"
    "#ifndef %s\n"
    "#define %s\n\n"
    "";

static const char *footer_fmt = ""
    "#endif // %s\n"
    "";

static const char *enum_header_fmt = ""
    "#ifdef %s_ENUM\n"
    "\n"
    "#undef %s_DEFS /* Do not allow also including the defs section */\n"
    "\n"
    "enum %s {\n"
    "";

static const char *enum_footer_fmt = ""
    "};\n"
    "\n"
    "#endif // %s_ENUM\n"
    "\n"
    "";

static const char *defs_header_fmt = ""
    "#if defined(%s_DEFS) || !defined(%s_ENUM)\n"
    "\n"
    "";

static const char *defs_footer_fmt = ""
    "\n"
    "#endif // defined(%s_DEFS) || !defined(%s_ENUM)\n"
    "\n"
    "";

static const char *lookup_header_fmt = ""
    "#ifdef %s_LOOKUP\n"
    "\n"
    "struct %s_entry {\n"
    "    const long value;\n"
    "    const char *def;\n"
    "};\n"
    "\n"
    "const struct %s_entry %s_lookup[] = {\n"
    "";

static const char *lookup_footer_fmt = ""
    "};\n"
    "\n"
    "#endif // %s_LOOKUP\n"
    "\n"
    "";

static const char *enum_entry_fmt = "    %s_%s = %zd,\n";
static const char *defs_entry_fmt = "#define %s_%s %zd\n";
static const char *lookup_entry_fmt = "    { %s_%s, \"%s\" },\n";
// clang-format on

const char *generate(struct deque *input, struct options *opts, const int argc, const char **argv)
{
    // Start with a giant buffer, just to avoid costly reallocations
    char *output = calloc(65536, sizeof(char));
    char *base = basename(opts->output_file);
    char *incg = usnake(base);
    char *lead = stem(incg, '_');
    char *enum_t = lsnake(opts->input_file);
    char *bufp = output;

    bufp = write_header(&bufp, incg, opts, argc, argv);
    bufp = write_enum(&bufp, input, lead, enum_t, opts);
    bufp = write_defs(&bufp, input, lead, opts);
    bufp = write_lookup(&bufp, input, lead, enum_t, opts);
    bufp = write_footer(&bufp, incg);

    free(incg);
    free(lead);
    free(base);
    free(enum_t);
    return output;
}

static char *write_header(char **output, const char *incg, struct options *opts, const int argc, const char **argv)
{
    char *bufp = *output;
    bufp += sprintf(bufp, header_p1_fmt, opts->input_file);

    int upper = opts->from_stdin ? argc : argc - 1;
    for (int i = 1; i < upper; i++) {
        bufp += sprintf(bufp, " %s", argv[i]);
    }

    bufp += sprintf(bufp, header_p2_fmt, incg, incg);
    return bufp;
}

static char *write_footer(char **output, const char *incg)
{
    char *bufp = *output;
    bufp += sprintf(bufp, footer_fmt, incg);
    return bufp;
}

static char *write_enum(char **output, struct deque *input, const char *lead, const char *enum_t, struct options *opts)
{
    char *bufp = *output;
    bufp += sprintf(bufp, enum_header_fmt, opts->preproc_guard, opts->preproc_guard, enum_t);

    struct entry_user user = {
        .bufp = &bufp,
        .it = opts->start_from,
        .lead = lead,
        .fmt = enum_entry_fmt,
    };
    deque_foreach_ftob(opts->prepend, write_member_entry, &user);
    deque_foreach_ftob(input, write_member_entry, &user);
    deque_foreach_ftob(opts->append, write_member_entry, &user);

    bufp += sprintf(bufp, enum_footer_fmt, opts->preproc_guard);

    return bufp;
}

static char *write_defs(char **output, struct deque *input, const char *lead, struct options *opts)
{
    char *bufp = *output;
    bufp += sprintf(bufp, defs_header_fmt, opts->preproc_guard, opts->preproc_guard);

    struct entry_user user = {
        .bufp = &bufp,
        .it = opts->start_from,
        .lead = lead,
        .fmt = defs_entry_fmt,
    };
    deque_foreach_ftob(opts->prepend, write_member_entry, &user);
    deque_foreach_ftob(input, write_member_entry, &user);
    deque_foreach_ftob(opts->append, write_member_entry, &user);

    bufp += sprintf(bufp, defs_footer_fmt, opts->preproc_guard, opts->preproc_guard);

    return bufp;
}

static char *write_lookup(char **output, struct deque *input, const char *lead, const char *enum_t, struct options *opts)
{
    char *bufp = *output;
    bufp += sprintf(bufp, lookup_header_fmt, opts->preproc_guard, enum_t, enum_t, enum_t);

    struct entry_user user = {
        .bufp = &bufp,
        .lead = lead,
        .fmt = lookup_entry_fmt,
    };

    // TODO: Unify these into one deque and sort the entries
    deque_foreach_ftob(opts->prepend, write_lookup_entry, &user);
    deque_foreach_ftob(input, write_lookup_entry, &user);
    deque_foreach_ftob(opts->append, write_lookup_entry, &user);

    bufp += sprintf(bufp, lookup_footer_fmt, opts->preproc_guard);

    return bufp;
}

static void write_member_entry(void *data, void *user_v)
{
    char *entry = strdup(data);
    struct entry_user *user = user_v;

    char *p;
    if ((p = strchr(entry, '='))) {
        char *lvalue = strndup(entry, p - entry - 1);
        char *rvalue = ltrim(p + 1);
        user->it = strtol(rvalue, NULL, 10);

        free(entry);
        free(rvalue);

        entry = lvalue;
    }

    char *tmp = entry;
    entry = usnake(entry);

    (*user->bufp) += sprintf(*user->bufp, user->fmt, user->lead, entry, user->it);
    user->it++;

    free(entry);
    free(tmp);
}

static void write_lookup_entry(void *data, void *user_v)
{
    char *entry = usnake(data);
    struct entry_user *user = user_v;
    (*user->bufp) += sprintf(*user->bufp, user->fmt, user->lead, entry, entry);
    free(entry);
}
