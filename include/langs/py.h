// SPDX-License-Identifier: MIT

#pragma once

#include <stdio.h>

#include "global.h"

void py_pregen(FILE *stream, sequence *seq, args *args);
void py_gen(FILE *stream, sequence *seq, args *args);
void py_postgen(FILE *stream, sequence *seq, args *args);
