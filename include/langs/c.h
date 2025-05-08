// SPDX-License-Identifier: MIT

#pragma once

#include <stdio.h>

#include "global.h"

#include "libs/vector.h"

void c_pregen(FILE *stream, vector *sequence, args *args);
void c_gen(FILE *stream, vector *sequence, args *args);
void c_postgen(FILE *stream, vector *sequence, args *args);
