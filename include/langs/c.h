// SPDX-License-Identifier: MIT

#pragma once

#include <stdio.h>

#include "global.h"

void c_pregen(FILE *stream, sequence *seq, args *args);
void c_gen(FILE *stream, sequence *seq, args *args);
void c_postgen(FILE *stream, sequence *seq, args *args);
