// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L // NOLINT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "metang.h" // meson-generated

#include "libs/clip.h"
#include "libs/strings.h"
#include "libs/vector.h"

typedef struct args {
    const char *lang;     // --lang   - defaults to "c"
    const char *guard;    // --guard  - defaults to "METANG"
    const char *tag;      // --tag    - defaults to basename of the input file
    const char *outfname; // --output - defaults to stdout
    const char *infname;  // <file>   - specify "-" to use stdin

    FILE *infile;
} args;

typedef struct seqelem {
    string symbol;
    long   value;
} seqelem;

#define BLOCK_SIZE 128

void   usage(FILE *stream);
args   parseargs(const int argc, const char **argv);
vector readseq(FILE *infile);

int main(int argc, const char **argv)
{
    // 1. Pick the generator for the specified language
    // 2. Process input lines into a sequence
    // 3. Pre-process the tag and guard using the generator
    // 4. Prepare the output stream
    // 5. Call the generator on the sequence and direct to the output stream

    args   args     = parseargs(argc, argv);
    vector sequence = readseq(args.infile);

    for (int i = 0; i < sequence.len; i++) {
        seqelem *elem = get(&sequence, seqelem, i);
        printf("%.*s -> %ld\n", fmtstring(elem->symbol), elem->value);
    }

    for (int i = 0; i < sequence.len; i++) free(get(&sequence, seqelem, i)->symbol.s);
    free(sequence.data);
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

vector readseq(FILE *infile)
{
    vector  sequence = newvec(seqelem, BLOCK_SIZE);
    bool    kill     = false;
    long    valit    = 0;
    char   *line     = null;
    size_t  linelen  = 0;
    ssize_t nread;

    while ((nread = getline(&line, &linelen, infile)) != -1) {
        seqelem *elem    = push(&sequence, seqelem);
        elem->symbol.s   = calloc(nread, 1);
        elem->symbol.len = nread - (line[nread - 1] == '\n'); // Do not copy the trailing newline

        memcpy(elem->symbol.s, line, elem->symbol.len);
        elem->symbol = strcut(elem->symbol, '#').head; // Trim any in-line comment

        // Handle direct value assignments
        strpair symval = strcut(elem->symbol, '=');
        if (symval.tail.len > 0) {
            char invalid = 0;
            valit        = strnum(symval.tail, 0, &invalid);

            if (invalid != '\0') {
                errF("invalid assignment value: “%.*s”", fmtstring(elem->symbol));
                valit = 0;
                kill  = true;
            }

            elem->symbol = symval.head;
        }

        elem->symbol = strrtrim(elem->symbol); // Trim trailing whitespace
        elem->value  = valit++;
    }

    free(line);
    if (kill) exit(EXIT_FAILURE);
    return sequence;
}
