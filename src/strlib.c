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
#include <stdlib.h>
#include <string.h>

char *strdup(const char *s)
{
    size_t len = strlen(s);
    char *dup = calloc(len + 1, sizeof(char));
    strcpy(dup, s);
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

char *lsnake(const char *s)
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

char *usnake(const char *s)
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