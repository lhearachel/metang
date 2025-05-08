#include "langs/c.h"

#include <stdio.h>

#include "global.h"

#include "libs/strings.h"
#include "libs/vector.h"

void c_pregen(FILE *stream, vector *sequence, args *args)
{
    unused(stream);
    unused(sequence);
    unused(args);
}

void c_gen(FILE *stream, vector *sequence, args *args)
{
    unused(args);

    for (int i = 0; i < sequence->len; i++) {
        seqelem *elem = get(sequence, seqelem, i);
        fprintf(stream, "%.*s -> %ld\n", fmtstring(elem->symbol), elem->value);
    }
}

void c_postgen(FILE *stream, vector *sequence, args *args)
{
    unused(stream);
    unused(sequence);
    unused(args);
}
