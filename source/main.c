// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "metang.h" // meson-generated

#include "libs/clip.h"

typedef struct args {
    const char *lang;     // --lang   - defaults to "c"
    const char *guard;    // --guard  - defaults to "METANG"
    const char *tag;      // --tag    - defaults to basename of the input file
    const char *outfname; // --output - defaults to stdout
    const char *infname;  // <file>   - specify "-" to use stdin

    FILE *infile;
} args;

void usage(FILE *stream);
args parseargs(const int argc, const char **argv);

int main(int argc, const char **argv)
{
    args args = parseargs(argc, argv);

    // 1. Pick the generator for the specified language
    // 2. Process input lines into a sequence
    // 3. Pre-process the tag and guard using the generator
    // 4. Prepare the output stream
    // 5. Call the generator on the sequence and direct to the output stream

    fclose(args.infile);
    return EXIT_SUCCESS;
}

void usage(FILE *stream)
{
    fprintf(stream, PROGRAM_NAME " - generate enumerable constants for multiple langauges\n");
    fprintf(stream, "\n");
    fprintf(stream, "Usage: " PROGRAM_NAME " [options] <file>\n");
    fprintf(stream, "       " PROGRAM_NAME " -h | --help\n");
    fprintf(stream, "       " PROGRAM_NAME " --version\n");
    fprintf(stream, "\n");
    fprintf(stream, "Options:\n");
    fprintf(stream, "  -o / --output <file>   Write generated content to a file.\n");
    fprintf(stream, "  -l / --lang <lang>     Generate enumerables for a specified language.\n");
    fprintf(stream, "  -t / --tag <tag>       Prefix generated enums and structs with <tag>.\n");
    fprintf(stream, "  -g / --guard <guard>   Prefix pre-processor conditionals with <guard>.\n");
}

args parseargs(const int argc, const char **argv)
{
    if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        exit(EXIT_SUCCESS);
    }

    if (strcmp(argv[1], "--version") == 0) {
        puts(PROGRAM_VERSION);
        exit(EXIT_SUCCESS);
    }

    args args = {
        .lang     = "c",
        .guard    = "METANG",
        .tag      = null,
        .outfname = null,
        .infname  = null,
        .infile   = null,
    };

    // clang-format off
    const clipopt options[] = {
        { .longopt = "lang",   .shortopt = 'l', .hasarg = H_reqarg, .starget = &args.lang     },
        { .longopt = "guard",  .shortopt = 'g', .hasarg = H_reqarg, .starget = &args.guard    },
        { .longopt = "tag",    .shortopt = 't', .hasarg = H_reqarg, .starget = &args.tag      },
        { .longopt = "output", .shortopt = 'o', .hasarg = H_reqarg, .starget = &args.outfname },
        { 0 }
    };

    const clippos arguments[] = {
        { .name = "file", .target = &args.infname },
        { 0 },
    };
    // clang-format on

    clip    clip = clipinit(argv);
    cliperr err  = cliparse(&clip, options, arguments, null);
    if (err != E_clip_none) die(clip.err, usage(stderr));

    args.infile = strcmp(args.infname, "-") != 0 ? fopen(args.infname, "rb") : stdin;
    if (args.infile == null) die("could not open input file “%s”", usage(stderr), args.infname);

    return args;
}
