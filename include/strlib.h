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

#ifndef METANG_STRLIB_H
#define METANG_STRLIB_H

#include <stddef.h>

char *strdup(const char *s);
char *strndup(const char *s, const size_t n);
char *basename(const char *path);
char *stem(const char *s, const char delim);
char *pascal(const char *s);
char *lsnake(const char *s, const char *extra);
char *usnake(const char *s, const char *extra);
char *ltrim(const char *s);
char *rtrim(const char *s);
char *trim(const char *s);

#endif // METANG_STRLIB_H
