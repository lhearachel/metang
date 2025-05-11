// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L // NOLINT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "metang.h" // meson-generated

#include "langs/c.h"

#include "libs/clip.h"
#include "libs/strings.h"
#include "libs/vector.h"

typedef struct gen {
    const char *lang;
    void (*prefunc)(FILE *stream, vector *sequence, args *args);
    void (*genfunc)(FILE *stream, vector *sequence, args *args);
    void (*postfunc)(FILE *stream, vector *sequence, args *args);
} gen;

static const gen generators[] = {
    { .lang = "c", .prefunc = c_pregen, .genfunc = c_gen, .postfunc = c_postgen },
    { 0 },
};

void       usage(FILE *stream);
args       parseargs(const int argc, const char **argv);
const gen *pickgen(const char *lang);
vector     readseq(FILE *infile);
FILE      *getfile(const char *fname, FILE *fdefault);

int main(int argc, const char **argv)
{
    args       args      = parseargs(argc, argv);
    const gen *generator = pickgen(args.lang);
    vector     sequence  = readseq(args.infile);
    FILE      *outfile   = getfile(args.outfname, stdout);

    generator->prefunc(outfile, &sequence, &args);
    generator->genfunc(outfile, &sequence, &args);
    generator->postfunc(outfile, &sequence, &args);

    for (int i = 0; i < sequence.len; i++) free(get(&sequence, seqelem, i)->symbol.s);
    free(sequence.data);
    free(args.guard.s);
    free(args.infbaseup.s);
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
    fprintf(stream, "  -b / --bitmask         Generate an enumerated bitmask.\n");
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

    args args    = { 0 };
    args.lang    = "c";
    args.inguard = "METANG";

    // clang-format off
    const clipopt options[] = {
        { .longopt = "lang",    .shortopt = 'l', .hasarg = H_reqarg, .starget = &args.lang     },
        { .longopt = "guard",   .shortopt = 'g', .hasarg = H_reqarg, .starget = &args.inguard  },
        { .longopt = "tag",     .shortopt = 't', .hasarg = H_reqarg, .starget = &args.intag    },
        { .longopt = "output",  .shortopt = 'o', .hasarg = H_reqarg, .starget = &args.outfname },
        { .longopt = "bitmask", .shortopt = 'b', .hasarg = H_noarg,  .ntarget = &args.bitmask  },
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

    if (strcmp(args.infname, "-") == 0) args.infname = null;
    args.infile = getfile(args.infname, stdin);

    return args;
}

const gen *pickgen(const char *lang)
{
    const gen *generator = &generators[0];
    for (; generator->lang != null && strcmp(generator->lang, lang) != 0; generator++);
    if (generator->lang == null) die("unrecognized lang “%s”", usage(stderr), lang);
    return generator;
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

FILE *getfile(const char *fname, FILE *fdefault)
{
    const char *mode = (fdefault == stdout || fdefault == stderr) ? "wb" : "rb";
    const char *type = (fdefault == stdout || fdefault == stderr) ? "output" : "input";
    FILE       *f    = fname ? fopen(fname, mode) : fdefault;
    if (f == null) {
        errF("could not open %s file “%s”", type, fname);
        usage(stderr);
        exit(EXIT_FAILURE);
    }

    return f;
}
