#include <stdio.h>
#include <stdlib.h>

#include "global.h"

#include "metang.h" // meson-generated

int main(int argc, const char *argv[])
{
    unused(argc);
    unused(argv);

    printf("%s - v%s\n", PROGRAM_NAME, PROGRAM_VERSION);
    return EXIT_SUCCESS;
}
