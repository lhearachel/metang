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

static char *strdup(const char *s);
static char *basename(const char *path);
static char *stem(const char *s, const char delim);
static char *snake(const char *s);
static char *upper_snake(const char *s);

// Write the "this file is generated" warning and the top-level include guard
static char *write_header(char **output, const char *incg, struct options *opts);

// Write the closer of the top-level include guard
static char *write_footer(char **output, const char *incg);

// Write the enum content section
static char *write_enum(char **output, struct deque *input, const char *lead, const char *enum_t, struct options *opts);

// Write the defs content section
static char *write_defs(char **output, struct deque *input, const char *lead, struct options *opts);

// Write the lookup content section
static char *write_lookup(char **output, struct deque *input, const char *lead, const char *enum_t);

static void write_enum_entry(void *data, void *user_v);
static void write_defs_entry(void *data, void *user_v);
static void write_lookup_entry(void *data, void *user_v);

static const char *header_fmt;
static const char *footer_fmt;
static const char *enum_header_fmt;
static const char *enum_footer_fmt;
static const char *defs_header_fmt;
static const char *defs_footer_fmt;
static const char *lookup_header_fmt;
static const char *lookup_footer_fmt;

const char *generate(struct deque *input, struct options *opts)
{
    // Start with a giant buffer, just to avoid costly reallocations
    char *output = calloc(65536, sizeof(char));
    char *base = basename(opts->output_file);
    char *incg = upper_snake(base);
    char *lead = stem(incg, '_');
    char *enum_t = snake(opts->input_file);
    char *bufp = output;

    bufp = write_header(&bufp, incg, opts);
    bufp = write_enum(&bufp, input, lead, enum_t, opts);
    bufp = write_defs(&bufp, input, lead, opts);
    bufp = write_lookup(&bufp, input, lead, enum_t);
    bufp = write_footer(&bufp, incg);

    free(incg);
    free(lead);
    return output;
}

static char *write_header(char **output, const char *incg, struct options *opts)
{
    char *bufp = *output;
    bufp += sprintf(bufp, header_fmt, opts->input_file, incg, incg);
    return bufp;
}

static char *write_footer(char **output, const char *incg)
{
    char *bufp = *output;
    bufp += sprintf(bufp, footer_fmt, incg);
    return bufp;
}

struct enum_entry_user {
    char **bufp;
    const char *lead;
};

static char *write_enum(char **output, struct deque *input, const char *lead, const char *enum_t, struct options *opts)
{
    char *bufp = *output;
    bufp += sprintf(bufp, enum_header_fmt, enum_t);

    char *first = upper_snake(deque_peek_f(input));
    bufp += sprintf(bufp, "    %s_%s = %zd,\n", lead, first, opts->start_from);

    struct enum_entry_user user = {
        .bufp = &bufp,
        .lead = lead,
    };
    deque_foreach_itob(input, 1, write_enum_entry, &user);

    bufp += sprintf(bufp, "%s\n", enum_footer_fmt);

    free(first);
    return bufp;
}

static void write_enum_entry(void *data, void *user_v)
{
    char *entry = upper_snake(data);
    struct enum_entry_user *user = user_v;
    (*user->bufp) += sprintf(*user->bufp, "    %s_%s,\n", user->lead, entry);
    free(entry);
}

struct defs_entry_user {
    char **bufp;
    const char *lead;
    ssize_t it;
};

static char *write_defs(char **output, struct deque *input, const char *lead, struct options *opts)
{
    char *bufp = *output;
    bufp += sprintf(bufp, "%s\n", defs_header_fmt);

    struct defs_entry_user user = {
        .bufp = &bufp,
        .lead = lead,
        .it = opts->start_from,
    };
    deque_foreach_ftob(input, write_defs_entry, &user);

    bufp += sprintf(bufp, "\n%s\n", defs_footer_fmt);

    return bufp;
}

static void write_defs_entry(void *data, void *user_v)
{
    char *entry = upper_snake(data);
    struct defs_entry_user *user = user_v;
    (*user->bufp) += sprintf(*user->bufp, "#define %s_%s %zd\n", user->lead, entry, user->it++);
    free(entry);
}

static char *write_lookup(char **output, struct deque *input, const char *lead, const char *enum_t)
{
    char *bufp = *output;
    bufp += sprintf(bufp, lookup_header_fmt, enum_t, enum_t, enum_t);

    struct enum_entry_user user = {
        .bufp = &bufp,
        .lead = lead,
    };
    deque_foreach_ftob(input, write_lookup_entry, &user);

    bufp += sprintf(bufp, "%s\n", lookup_footer_fmt);

    return bufp;
}

static void write_lookup_entry(void *data, void *user_v)
{
    char *entry = upper_snake(data);
    struct enum_entry_user *user = user_v;
    (*user->bufp) += sprintf(*user->bufp, "    { \"%s\", %s_%s },\n", entry, user->lead, entry);
    free(entry);
}

static char *strdup(const char *s)
{
    size_t len = strlen(s);
    char *dup = calloc(len + 1, sizeof(char));
    strcpy(dup, s);
    return dup;
}

static char *basename(const char *path)
{
    return stem(path, '/');
}

static char *stem(const char *s, const char delim)
{
    char *p = strrchr(s, delim);

    if (p == NULL) {
        return strdup(s);
    } else {
        return strdup(p + 1);
    }
}

static char *snake(const char *s)
{
    char c;
    char *new = strdup(s);
    for (size_t i = 0; (c = new[i]) != '\0'; i++) {
        if (isupper(c)) {
            new[i] = tolower(c);
        } else if (ispunct(c)) {
            new[i] = '_';
        }
    }

    return new;
}

static char *upper_snake(const char *s)
{
    char c;
    char *new = strdup(s);
    for (size_t i = 0; (c = new[i]) != '\0'; i++) {
        if (islower(c)) {
            new[i] = toupper(c);
        } else if (ispunct(c)) {
            new[i] = '_';
        }
    }

    return new;
}

// clang-format off
static const char *header_fmt = ""
    "/*\n"
    " * This file was generated by metang; DO NOT MODIFY IT!!\n"
    " * Source file: %s\n"
    " */\n"
    "#ifndef %s\n"
    "#define %s\n\n"
    "";

static const char *footer_fmt = ""
    "#endif // %s\n"
    "";

static const char *enum_header_fmt = ""
    "#ifdef METANG_ENUM\n"
    "\n"
    "#undef METANG_DEFS /* Do not allow also including the defs section */\n"
    "\n"
    "enum %s {\n"
    "";

static const char *enum_footer_fmt = ""
    "};\n"
    "\n"
    "#endif // METANG_ENUM\n"
    "";

static const char *defs_header_fmt = ""
    "#if defined(METANG_DEFS) || !defined(METANG_ENUM)\n"
    "";

static const char *defs_footer_fmt = ""
    "#endif // defined(METANG_DEFS) || !defined(METANG_ENUM)\n"
    "";

static const char *lookup_header_fmt = ""
    "#ifdef METANG_LOOKUP\n"
    "\n"
    "struct %s_entry {\n"
    "    const char *def;\n"
    "    const long value;\n"
    "};\n"
    "\n"
    "const struct %s_entry %s_lookup[] = {\n"
    "";

static const char *lookup_footer_fmt = ""
    "};\n"
    "\n"
    "#endif // METANG_LOOKUP\n"
    "";
// clang-format on
