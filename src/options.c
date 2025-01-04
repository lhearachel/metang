#include "options.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "meta.h"
#include "version.h"

#define OPT_FAIL(opts, r) \
    opts->result = r;     \
    FAIL_JUMP;

typedef struct {
    char *longopt;
    char shortopt;
    bool has_arg : 24;
    void (*handler)(options *opts, char *arg, void **jmpbuf);
} opthandler;

static void handle_bitmask(options *opts, char *arg, void **jmpbuf);
static void handle_allow_overrides(options *opts, char *arg, void **jmpbuf);
static void handle_append(options *opts, char *arg, void **jmpbuf);
static void handle_prepend(options *opts, char *arg, void **jmpbuf);
static void handle_start_from(options *opts, char *arg, void **jmpbuf);
static void handle_output(options *opts, char *arg, void **jmpbuf);
static void handle_leader(options *opts, char *arg, void **jmpbuf);
static void handle_tag_case(options *opts, char *arg, void **jmpbuf);
static void handle_tag_name(options *opts, char *arg, void **jmpbuf);
static void handle_preproc_guard(options *opts, char *arg, void **jmpbuf);

// clang-format off
static const opthandler opthandlers[] = {
    { "bitmask",         'B', false, handle_bitmask         },
    { "allow-overrides", 'D', false, handle_allow_overrides },
    { "append",          'a', true,  handle_append          },
    { "prepend",         'p', true,  handle_prepend         },
    { "start-from",      'n', true,  handle_start_from      },
    { "output",          'o', true,  handle_output          },
    { "leader",          'l', true,  handle_leader          },
    { "tag-case",        'c', true,  handle_tag_case        },
    { "tag-name",        't', true,  handle_tag_name        },
    { "preproc-guard",   'G', true,  handle_preproc_guard   },
    { 0 }, // must ALWAYS be last!
};

static const struct { char *arg; enum tag_case casing; } tag_casings[] = {
    { "snake",  TAG_SNAKE_CASE  },
    { "pascal", TAG_PASCAL_CASE },
    { 0 }, // must ALWAYS be last!
};

static const char *errmsg[] = {
    [OPTS_S]                     = "",
    [OPTS_F_UNRECOGNIZED_OPT]    = "Unrecognized option “%s”",
    [OPTS_F_OPT_MISSING_ARG]     = "Option “%s” missing argument",
    [OPTS_F_TOO_MANY_APPENDS]    = "Too many “--append” options",
    [OPTS_F_TOO_MANY_PREPENDS]   = "Too many “--prepend” options",
    [OPTS_F_NOT_AN_INTEGER]      = "Expected integer argument for option “%s”, but found “%s”",
    [OPTS_F_UNRECOGNIZED_CASING] = "Expected one of “snake” or “pascal” for option “%s”, but found “%s”",
};
// clang-format on

static inline char *chomp_argv(int *argc, char ***argv)
{
    char *arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}

static inline bool match(char *opt, char shortopt, char *longopt)
{
    bool check1 = (opt[0] == shortopt && opt[1] == '\0');
    bool check2 = (longopt != NULL && strcmp(opt + 1, longopt) == 0);
    return check1 || check2;
}

static inline void set_defaults(options *opts)
{
    opts->start = 0;
    opts->append_count = 0;
    opts->prepend_count = 0;
    opts->leader = NULL;
    opts->tag = NULL;
    opts->guard = "METANG";
    opts->outfile = NULL;
    opts->infile = NULL;
    opts->casing = TAG_SNAKE_CASE;
    opts->bitmask = false;
    opts->overrides = false;
    opts->help = false;
    opts->version = false;
    opts->to_stdout = false;
    opts->fr_stdin = false;
    opts->last_opt = NULL;
    opts->last_arg = NULL;
}

options *parseopts(int *argc, char ***argv)
{
    chomp_argv(argc, argv);
    if (*argc == 0) {
        return NULL;
    }

    options *opts = malloc(sizeof(options));

    // If an error occurs during parsing, then we will come back here and
    // immediately return. Individual handlers are responsible for invoking
    // `FAIL_JUMP(jmpbuf)` as needed.
    void *jmpbuf[5];
    if (__builtin_setjmp(jmpbuf)) {
        return opts;
    }

    set_defaults(opts);
    do {
        char *opt = chomp_argv(argc, argv);
        if (opt[0] != '-' || (opt[1] == '-' && opt[2] == '\0')) {
            break;
        }

        if (match(opt, 'h', "help")) {
            opts->help = true;
            break;
        }

        if (match(opt, 'v', "version")) {
            opts->version = true;
            break;
        }

        opts->last_opt = opt;
        opts->last_arg = NULL;
        usize i = 0;
        for (; i < lengthof(opthandlers); i++) {
            if (match(opt + 1, opthandlers[i].shortopt, opthandlers[i].longopt)) {
                char *arg = NULL;
                if (opthandlers[i].has_arg) {
                    if (*argc < 1) {
                        opts->result = OPTS_F_OPT_MISSING_ARG;
                        goto fail_jump;
                    }
                    arg = chomp_argv(argc, argv);
                    opts->last_arg = arg;
                }
                opthandlers[i].handler(opts, arg, jmpbuf);
                break;
            }
        }

        if (i == lengthof(opthandlers)) {
            opts->result = OPTS_F_UNRECOGNIZED_OPT;
            goto fail_jump;
        }
    } while (*argc > 0);

fail_jump:
    return opts;
}

void optserr(options *opts, char *buf)
{
    sprintf(buf, errmsg[opts->result], opts->last_opt, opts->last_arg);
}

static void handle_bitmask(options *opts, char *arg, void **jmpbuf)
{
    opts->bitmask = true;
}

static void handle_allow_overrides(options *opts, char *arg, void **jmpbuf)
{
    opts->overrides = true;
}

static void handle_append(options *opts, char *arg, void **jmpbuf)
{
    if (opts->append_count < MAX_ADDITIONAL_VALS) {
        opts->append[opts->append_count] = arg;
        opts->append_count++;
        return;
    }

    OPT_FAIL(opts, OPTS_F_TOO_MANY_APPENDS);
}

static void handle_prepend(options *opts, char *arg, void **jmpbuf)
{
    if (opts->prepend_count < MAX_ADDITIONAL_VALS) {
        opts->prepend[opts->prepend_count] = arg;
        opts->prepend_count++;
        return;
    }

    OPT_FAIL(opts, OPTS_F_TOO_MANY_PREPENDS);
}

static void handle_start_from(options *opts, char *arg, void **jmpbuf)
{
    errno = 0;
    opts->start = strtol(arg, NULL, 10);

    if (errno) {
        OPT_FAIL(opts, OPTS_F_NOT_AN_INTEGER);
    }
}

static void handle_output(options *opts, char *arg, void **jmpbuf)
{
    opts->outfile = arg;
}

static void handle_leader(options *opts, char *arg, void **jmpbuf)
{
    opts->leader = arg;
}

static void handle_tag_case(options *opts, char *arg, void **jmpbuf)
{
    usize i = 0;
    for (; i < lengthof(tag_casings); i++) {
        if (strcmp(arg, tag_casings[i].arg) == 0) {
            opts->casing = tag_casings[i].casing;
            break;
        }
    }

    if (i == lengthof(tag_casings)) {
        OPT_FAIL(opts, OPTS_F_UNRECOGNIZED_CASING);
    }
}

static void handle_tag_name(options *opts, char *arg, void **jmpbuf)
{
    opts->tag = arg;
}

static void handle_preproc_guard(options *opts, char *arg, void **jmpbuf)
{
    opts->guard = arg;
}

// clang-format off
const char *version = METANG_VERSION;

const char *tag_line = "metang - Generate multi-purpose C headers for enumerators";

const char *short_usage = "Usage: metang [options] [<input_file>]";

const char *options_section = ""
    "Options:\n"
    "  -a, --append <entry>         Append <entry> to the input listing.\n"
    "  -p, --prepend <entry>        Prepend <entry> to the input listing.\n"
    "  -n, --start-from <number>    Start enumeration from <number>.\n"
    "  -o, --output <file>          Write output to <file>.\n"
    "  -l, --leader <leader>        Use <leader> as a prefix for generated symbols.\n"
    "  -t, --tag-name <name>        Use <name> as the base tag for enums and lookup\n"
    "                               tables.\n"
    "  -c, --tag-case <case>        Customize the casing of generated tags for enums\n"
    "                               and lookup tables. Options: snake, pascal\n"
    "  -G, --preproc-guard <guard>  Use <guard> as a prefix for conditional\n"
    "                               preprocessor directives.\n"
    "  -B, --bitmask                If specified, generate symbols for a bitmask.\n"
    "                               This option is incompatible with the “-D” and\n"
    "                               “-n” options.\n"
    "  -D, --allow-overrides        If specified, allow direct value-assignment.\n"
    "  -h, --help                   Display this help text and exit.\n"
    "  -v, --version                Display the program version number and exit."
    "";
// clang-format on
