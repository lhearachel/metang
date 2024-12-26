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

#include "strlib.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strdup(const char *s)
{
    size_t len = strlen(s);
    char *dup = calloc(len + 1, sizeof(char));
    strcpy(dup, s);
    return dup;
}

char *strndup(const char *s, const size_t n)
{
    char *dup = calloc(n + 1, sizeof(char));
    strncpy(dup, s, n);
    return dup;
}

char *basename(const char *path)
{
    char *s = strrchr(path, '/');

    if (s == NULL) {
        return strdup(path);
    } else {
        return strdup(s + 1);
    }
}

char *stem(const char *s, const char delim)
{
    char *p = strchr(s, delim);

    if (p == NULL) {
        return strdup(s);
    } else {
        char *t = calloc(p - s + 1, sizeof(char));
        strncpy(t, s, p - s);
        return t;
    }
}

char *pascal(const char *s)
{
    char *p = calloc(strlen(s) + 1, sizeof(char));

    if (!isalpha(*s)) {
        strcpy(p, s);
        return p;
    }

    char *i = p;
    *i = toupper(*s);
    s++;
    i++;

    while (*s) {
        if (ispunct(*s) && islower(*(s + 1))) {
            s++;
            *i = toupper(*s);
        } else {
            *i = *s;
        }
        s++;
        i++;
    }

    return p;
}

static bool replace_uscore(const char c, const char *extra)
{
    return c && (isspace(c) || c == '-' || c == '_' || (extra && strchr(extra, c)));
}

char *lsnake(const char *s, const char *extra)
{
    if (!s) {
        return calloc(1, sizeof(char));
    }

    char *new = calloc(strlen(s) + 1, sizeof(char));
    char *p = new;
    while (*s) {
        if (replace_uscore(*s, extra)) {
            *p = '_';
        } else if (ispunct(*s)) {
            s++;
            continue;
        } else if (isupper(*s)) {
            *p = tolower(*s);
        } else {
            *p = *s;
        }

        p++;
        s++;
    }

    return new;
}

char *usnake(const char *s, const char *extra)
{
    if (!s) {
        return calloc(1, sizeof(char));
    }

    char *new = calloc(strlen(s) + 1, sizeof(char));
    char *p = new;
    while (*s) {
        if (replace_uscore(*s, extra)) {
            *p = '_';
        } else if (ispunct(*s)) {
            s++;
            continue;
        } else if (islower(*s)) {
            *p = toupper(*s);
        } else {
            *p = *s;
        }

        p++;
        s++;
    }

    return new;
}

char *ltrim(const char *s)
{
    size_t i = 0;
    while (isspace(s[i])) {
        i++;
    }

    if (s[i] == '\0') {
        return calloc(1, sizeof(char));
    }

    return strdup(s + i);
}

char *rtrim(const char *s)
{
    size_t i = strlen(s);
    while (i > 0 && isspace(s[i - 1])) {
        i--;
    }

    if (i == 0) {
        return calloc(1, sizeof(char));
    }

    char *p = calloc(i + 2, sizeof(char));
    strncpy(p, s, i + 1);
    return p;
}

char *trim(const char *s)
{
    char *t = ltrim(s);
    char *p = rtrim(t);

    free(t);
    return p;
}
