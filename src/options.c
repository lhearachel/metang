#include "options.h"

#include <stdio.h>
#include <string.h>

#include "meta.h"

typedef struct {
    char *longopt;
    char shortopt;
    bool has_arg : 24;
    bool (*handler)(options *opts, char *arg);
} opthandler;

static bool handle_bitmask(options *opts, char *arg);
static bool handle_allow_overrides(options *opts, char *arg);
static bool handle_append(options *opts, char *arg);
static bool handle_prepend(options *opts, char *arg);
static bool handle_start_from(options *opts, char *arg);
static bool handle_output(options *opts, char *arg);
static bool handle_leader(options *opts, char *arg);
static bool handle_tag_case(options *opts, char *arg);
static bool handle_tag_name(options *opts, char *arg);
static bool handle_preproc_guard(options *opts, char *arg);

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
    initopts(opts);

    char *opt = "";
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
            opts->result = OPTS_F_UNRECOGNIZED_OPT;
            return false;
        }

        char *arg = NULL;
        if (opthandlers[i].has_arg) {
            if (*argc < 1) {
                opts->result = OPTS_F_OPT_MISSING_ARG;
                return false;
            }

            arg = chomp_argv(argc, argv);
            opts->last_arg = arg;
        }
        if (!opthandlers[i].handler(opts, arg)) {
            return false;
        }
    }

    opts->to_stdout = opts->outfile == NULL;
    opts->fr_stdin = *argc == 0 && (opt[0] == '\0' || isopt(opt));
    if (!opts->fr_stdin) {
        opts->infile = opt;
    }

    return true;
}

void optserr(options *opts, char *buf)
{
    sprintf(buf, errmsg[opts->result], opts->last_opt, opts->last_arg);
}

static bool handle_bitmask(options *opts, char *arg)
{
    opts->bitmask = true;
    return true;
}

static bool handle_allow_overrides(options *opts, char *arg)
{
    opts->overrides = true;
    return true;
}

static bool handle_append(options *opts, char *arg)
{
    if (opts->append_count < MAX_ADDITIONAL_VALS) {
        opts->append[opts->append_count] = arg;
        opts->append_count++;
        return true;
    }

    opts->result = OPTS_F_TOO_MANY_APPENDS;
    return false;
}

static bool handle_prepend(options *opts, char *arg)
{
    if (opts->prepend_count < MAX_ADDITIONAL_VALS) {
        opts->prepend[opts->prepend_count] = arg;
        opts->prepend_count++;
        return true;
    }

    opts->result = OPTS_F_TOO_MANY_PREPENDS;
    return false;
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

static bool handle_start_from(options *opts, char *arg)
{
    if (decstol(arg, &opts->start)) {
        return true;
    }

    opts->result = OPTS_F_NOT_AN_INTEGER;
    return false;
}

static bool handle_output(options *opts, char *arg)
{
    opts->outfile = arg;
    return true;
}

static bool handle_leader(options *opts, char *arg)
{
    opts->leader = arg;
    return true;
}

static bool handle_tag_case(options *opts, char *arg)
{
    for (usize i = 0; i < lengthof(tag_casings); i++) {
        if (strcmp(arg, tag_casings[i].arg) == 0) {
            opts->casing = tag_casings[i].casing;
            return true;
        }
    }

    opts->result = OPTS_F_UNRECOGNIZED_CASING;
    return false;
}

static bool handle_tag_name(options *opts, char *arg)
{
    opts->tag = arg;
    return true;
}

static bool handle_preproc_guard(options *opts, char *arg)
{
    opts->guard = arg;
    return true;
}
