// SPDX-License-Identifier: MIT

#pragma once

#include <stdio.h>

#include "global.h"

void cpp_pregen(FILE *stream, sequence *seq, args *args);
void cpp_gen(FILE *stream, sequence *seq, args *args);
void cpp_postgen(FILE *stream, sequence *seq, args *args);
