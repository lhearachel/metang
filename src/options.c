#include "options.h"

#include <stdio.h>
#include <string.h>

#include "meta.h"
#include "strbuf.h"

typedef struct {
    str longopt;
    char shortopt;
    bool has_arg : 24;
    bool (*handler)(options *opts, str *arg);
} opthandler;

static bool handle_bitmask(options *opts, str *arg);
static bool handle_allow_overrides(options *opts, str *arg);
static bool handle_append(options *opts, str *arg);
static bool handle_prepend(options *opts, str *arg);
static bool handle_start_from(options *opts, str *arg);
static bool handle_output(options *opts, str *arg);
static bool handle_leader(options *opts, str *arg);
static bool handle_tag_case(options *opts, str *arg);
static bool handle_tag_name(options *opts, str *arg);
static bool handle_preproc_guard(options *opts, str *arg);

// clang-format off
static const str help = strnew("help");
static const str vers = strnew("version");

static const opthandler opthandlers[] = {
    { strnew("bitmask"),         'B', false, handle_bitmask         },
    { strnew("allow-overrides"), 'D', false, handle_allow_overrides },
    { strnew("append"),          'a', true,  handle_append          },
    { strnew("prepend"),         'p', true,  handle_prepend         },
    { strnew("start-from"),      'n', true,  handle_start_from      },
    { strnew("output"),          'o', true,  handle_output          },
    { strnew("leader"),          'l', true,  handle_leader          },
    { strnew("tag-case"),        'c', true,  handle_tag_case        },
    { strnew("tag-name"),        't', true,  handle_tag_name        },
    { strnew("preproc-guard"),   'G', true,  handle_preproc_guard   },
    { strZ,                      ' ', false, NULL                   }, // must ALWAYS be last!
};

static const struct { str arg; enum tag_case casing; } tag_casings[] = {
    { strnew("snake"),  TAG_SNAKE_CASE  },
    { strnew("pascal"), TAG_PASCAL_CASE },
    { strZ,             0               }, // must ALWAYS be last!
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

static inline str chomp_argv(int *argc, char ***argv)
{
    str arg = strnew(**argv, strlen(**argv));
    (*argc)--;
    (*argv)++;
    return arg;
}

static inline bool isopt(const str *s)
{
    return s->buf[0] == '-' && !(s->buf[1] == '-' && s->len == 2);
}

static inline bool match(const str *opt, char shortopt, const str *longopt)
{
    return (opt->len == 1 && opt->buf[0] == shortopt)
        || streq(opt, longopt);
}

static inline void initopts(options *opts)
{
    opts->result = OPTS_S;
    opts->last_opt = strZ;
    opts->last_arg = strZ;

    opts->append_count = 0;
    opts->prepend_count = 0;

    opts->start = 0;
    opts->leader = strZ;
    opts->tag = strZ;
    opts->guard = strnew("METANG");
    opts->outfile = strZ;
    opts->infile = strZ;

    opts->casing = TAG_SNAKE_CASE;
    opts->bitmask = false;
    opts->overrides = false;
    opts->help = false;
    opts->version = false;
}

bool parseopts(int *argc, char ***argv, options *opts)
{
    initopts(opts);

    str opt = strZ;
    str cutopt = strZ;
    while (*argc > 0 && (opt = chomp_argv(argc, argv)).len > 0 && isopt(&opt)) {
        cutopt = strrcut(&opt, '-').tail;
        opts->help = match(&cutopt, 'h', &help);
        opts->version = match(&cutopt, 'v', &vers);
        if (opts->help || opts->version) {
            break;
        }

        usize i = 0;
        opts->last_opt = opt;
        opts->last_arg = strZ;
        while (opthandlers[i].shortopt != ' '
               && !match(&cutopt, opthandlers[i].shortopt, &opthandlers[i].longopt)) {
            i++;
        }

        if (opthandlers[i].longopt.len == 0) {
            opts->result = OPTS_F_UNRECOGNIZED_OPT;
            return false;
        }

        str arg = strZ;
        if (opthandlers[i].has_arg) {
            if (*argc < 1) {
                opts->result = OPTS_F_OPT_MISSING_ARG;
                return false;
            }

            arg = chomp_argv(argc, argv);
            opts->last_arg = arg;
        }
        if (!opthandlers[i].handler(opts, &arg)) {
            return false;
        }
    }

    // If argv was not fully parsed, then we must have hit a terminating `--`.
    // If argv *was* fully parsed, then the final option must either not exist
    // (length 0) or be a required positional argument (and thus not an option).
    if (*argc != 0 || (opt.len != 0 && !isopt(&opt))) {
        opts->infile = opt;
    }

    return true;
}

void optserr(options *opts, char *buf)
{
    sprintf(buf, errmsg[opts->result], opts->last_opt, opts->last_arg);
}

static bool handle_bitmask(options *opts, str *arg)
{
    opts->bitmask = true;
    return true;
}

static bool handle_allow_overrides(options *opts, str *arg)
{
    opts->overrides = true;
    return true;
}

static bool handle_append(options *opts, str *arg)
{
    if (opts->append_count < MAX_ADDITIONAL_VALS) {
        opts->append[opts->append_count] = strnewp(arg);
        opts->append_count++;
        return true;
    }

    opts->result = OPTS_F_TOO_MANY_APPENDS;
    return false;
}

static bool handle_prepend(options *opts, str *arg)
{
    if (opts->prepend_count < MAX_ADDITIONAL_VALS) {
        opts->prepend[opts->prepend_count] = strnewp(arg);
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

static bool handle_start_from(options *opts, str *arg)
{
    if (decstol(arg->buf, &opts->start)) {
        return true;
    }

    opts->result = OPTS_F_NOT_AN_INTEGER;
    return false;
}

static bool handle_output(options *opts, str *arg)
{
    opts->outfile = strnewp(arg);
    return true;
}

static bool handle_leader(options *opts, str *arg)
{
    opts->leader = strnewp(arg);
    return true;
}

static bool handle_tag_case(options *opts, str *arg)
{
    for (usize i = 0; tag_casings[i].arg.len > 0; i++) {
        if (streq(arg, &tag_casings[i].arg)) {
            opts->casing = tag_casings[i].casing;
            return true;
        }
    }

    opts->result = OPTS_F_UNRECOGNIZED_CASING;
    return false;
}

static bool handle_tag_name(options *opts, str *arg)
{
    opts->tag = strnewp(arg);
    return true;
}

static bool handle_preproc_guard(options *opts, str *arg)
{
    opts->guard = strnewp(arg);
    return true;
}
