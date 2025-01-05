#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "meta.h"
#include "version.h"

#define OPT_EXCEPT(jb, opts, r) \
    opts->result = r;           \
    EXCEPT(jb);

#define OPT_FAIL(opts, r) \
    opts->result = r;     \
    return false;

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

static inline bool isopt(char *s)
{
    return s[0] == '-' && !(s[1] == '-' && s[2] == '\0');
}

static inline bool match(char *opt, char shortopt, char *longopt)
{
    return (opt[0] == shortopt && opt[1] == '\0')
        || (longopt != NULL && strcmp(opt + 1, longopt) == 0);
}

static inline void initopts(options *opts)
{
    opts->result = OPTS_S;
    opts->last_opt = NULL;
    opts->last_arg = NULL;

    opts->append_count = 0;
    opts->prepend_count = 0;

    opts->start = 0;
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
}

bool parseopts(int *argc, char ***argv, options *opts)
{
    chomp_argv(argc, argv);
    if (*argc == 0) {
        return false;
    }

    initopts(opts);

    CATCH(jmpbuf, {
        return false;
    });

    char *opt;
    while (*argc > 0 && (opt = chomp_argv(argc, argv)) && isopt(opt)) {
        opts->help = match(opt + 1, 'h', "help");
        opts->version = match(opt + 1, 'v', "version");
        if (opts->help || opts->version) {
            break;
        }

        usize i = 0;
        opts->last_opt = opt;
        opts->last_arg = NULL;
        while (i < lengthof(opthandlers) && !match(opt + 1, opthandlers[i].shortopt, opthandlers[i].longopt)) {
            i++;
        }

        if (i == lengthof(opthandlers)) {
            OPT_FAIL(opts, OPTS_F_UNRECOGNIZED_OPT);
        }

        char *arg = NULL;
        if (opthandlers[i].has_arg) {
            if (*argc < 1) {
                OPT_FAIL(opts, OPTS_F_OPT_MISSING_ARG);
            }

            arg = chomp_argv(argc, argv);
            opts->last_arg = arg;
        }
        opthandlers[i].handler(opts, arg, jmpbuf);
    }

    opts->to_stdout = opts->outfile == NULL;
    opts->fr_stdin = strcmp(opt, "--") == 0;
    if (!opts->fr_stdin) {
        opts->infile = opt;
    }

    return true;
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

    OPT_EXCEPT(jmpbuf, opts, OPTS_F_TOO_MANY_APPENDS);
}

static void handle_prepend(options *opts, char *arg, void **jmpbuf)
{
    if (opts->prepend_count < MAX_ADDITIONAL_VALS) {
        opts->prepend[opts->prepend_count] = arg;
        opts->prepend_count++;
        return;
    }

    OPT_EXCEPT(jmpbuf, opts, OPTS_F_TOO_MANY_PREPENDS);
}

static inline bool decstol(const char *s, long *l)
{
    bool neg = false;
    if (*s == '-') {
        neg = true;
        s++;
    }

    *l = 0;
    for (; *s != '\0'; s++) {
        if (*s >= '0' && *s <= '9') {
            *l = (*l * 10) + (*s - '0');
        } else {
            return false;
        }
    }

    if (neg) {
        *l = *l * -1;
    }
    return true;
}

static void handle_start_from(options *opts, char *arg, void **jmpbuf)
{
    if (!decstol(arg, &opts->start)) {
        OPT_EXCEPT(jmpbuf, opts, OPTS_F_NOT_AN_INTEGER);
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
        OPT_EXCEPT(jmpbuf, opts, OPTS_F_UNRECOGNIZED_CASING);
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
