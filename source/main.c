// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L // NOLINT

#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "metang.h" // meson-generated

#include "langs/c.h"
#include "langs/cpp.h"
#include "langs/py.h"

#include "libs/clip.h"
#include "libs/strings.h"
#include "libs/vector.h"

// clang-format off
static const gen generators[] = {
    { .lang = "c",   .ext = ".h",   .prefunc = c_pregen,   .genfunc = c_gen,   .postfunc = c_postgen   },
    { .lang = "cpp", .ext = ".hpp", .prefunc = cpp_pregen, .genfunc = cpp_gen, .postfunc = cpp_postgen },
    { .lang = "py",  .ext = ".py",  .prefunc = py_pregen,  .genfunc = py_gen,  .postfunc = py_postgen  },
    { 0 },
};
// clang-format on

void       usage(FILE *stream);
args       parseargs(const int argc, const char **argv);
const gen *pickgen_byext(const char *ext);
const gen *pickgen_bylang(const char *lang);
sequence   readseq(FILE *infile, bool bitmask);
FILE      *getfile(const char *fname, FILE *fdefault);

int main(int argc, const char **argv)
{
    args     args     = parseargs(argc, argv);
    sequence sequence = readseq(args.infile, args.bitmask);
    FILE    *outfile  = getfile(args.outfname, stdout);

    args.generator->prefunc(outfile, &sequence, &args);
    args.generator->genfunc(outfile, &sequence, &args);
    args.generator->postfunc(outfile, &sequence, &args);

    for (int i = 0; i < sequence.elems.len; i++) free(get(&sequence.elems, seqelem, i)->symbol.s);
    free(sequence.elems.data);
    free(args.guard.s);
    free(args.outfbaseup.s);
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
    fprintf(stream, "  -b / --bitmask         Generate an enumerated bitmask.\n");
    fprintf(stream, "  -o / --output <file>   Write generated content to a file.\n");
    fprintf(stream, "  -l / --lang <lang>     Generate enumerables for a specified language.\n");
    fprintf(stream, "  -t / --tag <tag>       Prefix generated enums and structs with <tag>.\n");
    fprintf(stream, "  -g / --guard <guard>   Prefix pre-processor conditionals with <guard>.\n");
    fprintf(stream, "  -s / --sized <size>    Bind the size and sign of the enum, if supported.\n");
    fprintf(stream, "                         e.g. in C, 8 binds to uint8_t, -8 to int8_t, etc.\n");
    fprintf(stream, "\n");
    fprintf(stream, "Languages Supported:\n");
    fprintf(stream, "  c     C enum, #defines, and a binary-searchable member-lookup table.\n");
    fprintf(stream, "  cpp   C++ enum with a member-lookup table using std::map.\n");
    fprintf(stream, "  py    Python class derived from enum.IntEnum or enum.IntFlag.\n");
}

args parseargs(const int argc, const char **argv)
{
    if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        exit(EXIT_SUCCESS);
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        puts(PROGRAM_VERSION);
        exit(EXIT_SUCCESS);
    }

    args args    = { 0 };
    args.inguard = "METANG";

    // clang-format off
    const clipopt options[] = {
        { .longopt = "lang",    .shortopt = 'l', .hasarg = H_reqarg, .starget = &args.lang     },
        { .longopt = "guard",   .shortopt = 'g', .hasarg = H_reqarg, .starget = &args.inguard  },
        { .longopt = "sized",   .shortopt = 's', .hasarg = H_reqarg, .starget = &args.insized  },
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

    const char *infname  = args.infname ? (char *)args.infname : "stdin";
    const char *outfname = args.outfname ? (char *)args.outfname : "stdout";
    const char *outext   = strrchr(outfname, '.');
    args.infbase         = strmake(basename((char *)infname));
    args.outfbaseup      = strupper(strmake(basename((char *)outfname)));
    args.guard           = strupper(string(args.inguard, strlen(args.inguard)));
    args.tag             = args.intag ? strmake(args.intag) : strcut(args.infbase, '.').head;
    args.generator       = args.lang ? pickgen_bylang(args.lang) : pickgen_byext(outext);

    args.sizesign = S_unbound;
    char invalid  = '\0';
    long sizesign = args.insized ? strnum(strmake(args.insized), 0, &invalid) : 0;
    if (invalid != '\0') die("non-numeric size binding: “%s”", usage(stderr), args.insized);
    switch (sizesign) {
    case 0:   args.sizesign = S_unbound; break;
    case -8:  args.sizesign = S_signed_8bit; break;
    case -16: args.sizesign = S_signed_16bit; break;
    case -32: args.sizesign = S_signed_32bit; break;
    case -64: args.sizesign = S_signed_64bit; break;
    case 8:   args.sizesign = S_unsigned_8bit; break;
    case 16:  args.sizesign = S_unsigned_16bit; break;
    case 32:  args.sizesign = S_unsigned_32bit; break;
    case 64:  args.sizesign = S_unsigned_64bit; break;
    default:  die("unrecognized size binding: “%s”", usage(stderr), args.insized);
    }

    return args;
}

const gen *pickgen_byext(const char *ext)
{
    ext = ext ? ext : ".h";

    const gen *generator = &generators[0];
    for (; generator->ext != null && strcmp(generator->ext, ext) != 0; generator++);
    if (generator->ext == null) {
        errF("unrecognized output extension lang “%s”; defaulting to C", ext);
        generator = pickgen_bylang("c");
    }

    return generator;
}

const gen *pickgen_bylang(const char *lang)
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
        elem->symbol = strrtrim(strltrim(strcut(elem->symbol, '#').head)); // Trim out line comments
        if (elem->symbol.len <= 0) {
            free(elem->symbol.s);
            pop(&elems, seqelem);
            continue;
        }

        // Handle direct value assignments
        strpair symval = strcut(elem->symbol, '=');
        symval.head    = strrtrim(symval.head);
        if (symval.head.len <= 0) {
            errF("no symbol given for direct assignment: “%.*s”", fmtstring(elem->symbol));
            free(elem->symbol.s);
            pop(&elems, seqelem);
            kill = true;
            continue;
        }

        symval.tail = strltrim(symval.tail);
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
