// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L // NOLINT

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "metang.h" // meson-generated

#include "langs/c.h"
#include "langs/py.h"

#include "libs/clip.h"
#include "libs/strings.h"
#include "libs/vector.h"

typedef struct gen {
    const char *lang;
    void (*prefunc)(FILE *stream, sequence *seq, args *args);
    void (*genfunc)(FILE *stream, sequence *seq, args *args);
    void (*postfunc)(FILE *stream, sequence *seq, args *args);
} gen;

// clang-format off
static const gen generators[] = {
    { .lang = "c",  .prefunc = c_pregen,  .genfunc = c_gen,  .postfunc = c_postgen  },
    { .lang = "py", .prefunc = py_pregen, .genfunc = py_gen, .postfunc = py_postgen },
    { 0 },
};
// clang-format on

void       usage(FILE *stream);
args       parseargs(const int argc, const char **argv);
const gen *pickgen(const char *lang);
sequence   readseq(FILE *infile, bool bitmask);
FILE      *getfile(const char *fname, FILE *fdefault);

int main(int argc, const char **argv)
{
    args       args      = parseargs(argc, argv);
    const gen *generator = pickgen(args.lang);
    sequence   sequence  = readseq(args.infile, args.bitmask);
    FILE      *outfile   = getfile(args.outfname, stdout);

    generator->prefunc(outfile, &sequence, &args);
    generator->genfunc(outfile, &sequence, &args);
    generator->postfunc(outfile, &sequence, &args);

    for (int i = 0; i < sequence.elems.len; i++) free(get(&sequence.elems, seqelem, i)->symbol.s);
    free(sequence.elems.data);
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

    const char *infname = args.infname ? (char *)args.infname : "stdin";
    args.infbase        = strmake(basename((char *)infname));
    args.infbaseup      = strupper(args.infbase);
    args.guard          = strupper(string(args.inguard, strlen(args.inguard)));
    args.tag            = args.intag ? strmake(args.intag) : strcut(args.infbase, '.').head;

    return args;
}

const gen *pickgen(const char *lang)
{
    const gen *generator = &generators[0];
    for (; generator->lang != null && strcmp(generator->lang, lang) != 0; generator++);
    if (generator->lang == null) die("unrecognized lang “%s”", usage(stderr), lang);
    return generator;
}

sequence readseq(FILE *infile, bool bitmask)
{
    vector  elems     = newvec(seqelem, BLOCK_SIZE);
    bool    kill      = false;
    long    valit     = 0;
    long    maxsymlen = 0;
    char   *line      = null;
    size_t  linelen   = 0;
    ssize_t nread;

    while ((nread = getline(&line, &linelen, infile)) != -1) {
        seqelem *elem    = push(&elems, seqelem);
        elem->symbol.s   = calloc(nread, 1);
        elem->symbol.len = nread - (line[nread - 1] == '\n'); // Do not copy the trailing newline

        memcpy(elem->symbol.s, line, elem->symbol.len);
        elem->symbol = strcut(elem->symbol, '#').head; // Trim any in-line comment

        // Handle direct value assignments
        strpair symval = strcut(elem->symbol, '=');
        symval.tail    = strrtrim(strltrim(symval.tail));
        if (symval.tail.len > 0) {
            if (bitmask) {
                errF(
                    "value assignment not allowed in bitmask mode: “%.*s”",
                    fmtstring(elem->symbol)
                );
                kill = true;
                goto sethead;
            }

            if (alpha(symval.tail.s[0]) || symval.tail.s[0] == '_') {
                // TODO: Replace naive linear search with a map lookup
                int i = 0;
                for (; i < elems.len; i++) {
                    seqelem *elem = get(&elems, seqelem, i);
                    if (strequ(elem->symbol, symval.tail)) {
                        valit = elem->value;
                        break;
                    }
                }

                if (i == elems.len) {
                    errF("unknown back-ref assignment: “%.*s”", fmtstring(elem->symbol));
                    valit = 0;
                    kill  = true;
                }
            } else {
                // Parse the number
                char invalid = 0;
                valit        = strnum(symval.tail, 0, &invalid);

                if (invalid != '\0') {
                    errF("invalid assignment value: “%.*s”", fmtstring(elem->symbol));
                    valit = 0;
                    kill  = true;
                }
            }

        sethead:
            elem->symbol = symval.head;
        }

        elem->symbol = strrtrim(elem->symbol); // Trim trailing whitespace
        elem->value  = valit++;
        maxsymlen    = maxsymlen >= elem->symbol.len ? maxsymlen : elem->symbol.len;
    }

    free(line);
    if (kill) exit(EXIT_FAILURE);
    return (sequence){ .elems = elems, .maxsymlen = maxsymlen };
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
