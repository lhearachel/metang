#include "options.h"

#include <stdio.h>
#include <string.h>

#include "generator.h"
#include "meta.h"
#include "strbuf.h"

typedef struct opthandler {
    str longopt;
    char shortopt;
    bool has_arg : 8;
    u32 mode     : 16;
    bool (*handler)(options *opts, str *arg);
} opthandler;

typedef struct opterrmsg {
    str fmt;
    usize argc;
} opterrmsg;

static bool handle_append(options *opts, str *arg);
static bool handle_prepend(options *opts, str *arg);
static bool handle_start_from(options *opts, str *arg);
static bool handle_output(options *opts, str *arg);
static bool handle_leader(options *opts, str *arg);
static bool handle_tag_name(options *opts, str *arg);
static bool handle_guard(options *opts, str *arg);
static bool handle_lang(options *opts, str *arg);

// clang-format off
static const opthandler opthandlers[] = {
    { strnew("append"),          'a', true,  OPTS_M_ENUM, handle_append          },
    { strnew("prepend"),         'p', true,  OPTS_M_ENUM, handle_prepend         },
    { strnew("start-from"),      'n', true,  OPTS_M_ENUM, handle_start_from      },
    { strnew("output"),          'o', true,  OPTS_M_ANY,  handle_output          },
    { strnew("leader"),          'l', true,  OPTS_M_ANY,  handle_leader          },
    { strnew("tag-name"),        't', true,  OPTS_M_ANY,  handle_tag_name        },
    { strnew("guard"),           'G', true,  OPTS_M_ANY,  handle_guard           },
    { strnew("lang"),            'L', true,  OPTS_M_ANY,  handle_lang            },
    { strZ,                      ' ', false, OPTS_M_NONE, NULL                   }, // must ALWAYS be last!
};

static const opterrmsg errmsg[] = {
    [OPTS_S]                     = { strZ,                                                                          0 },
    [OPTS_F_UNRECOGNIZED_OPT]    = { strnew("Unrecognized option “%s”"),                                            1 },
    [OPTS_F_OPT_MISSING_ARG]     = { strnew("Option “%s” missing argument"),                                        1 },
    [OPTS_F_TOO_MANY_APPENDS]    = { strnew("Too many “--append” options; limit: 16"),                              0 },
    [OPTS_F_TOO_MANY_PREPENDS]   = { strnew("Too many “--prepend” options; limit: 16"),                             0 },
    [OPTS_F_NOT_AN_INTEGER]      = { strnew("Expected integer argument for option “%s”, but found “%s”"),           2 },
    [OPTS_F_UNRECOGNIZED_LANG]   = { strnew("Unexpected value for option “%s” argument “%s”"),                      2 },
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

    opts->flags = false;

    opts->lang = strnew("c");
    opts->genf = 0;
}

bool parseopts(int *argc, char ***argv, options *opts)
{
    initopts(opts);

    str opt = strZ;
    str chopt = strZ;
    while (*argc > 0 && (opt = chomp_argv(argc, argv)).len > 0 && isopt(&opt)) {
        usize i = 0;
        opts->last_opt = opt;
        opts->last_arg = strZ;
        chopt = strchop(&opt, '-');
        while (opthandlers[i].shortopt != ' '
               && (!(opthandlers[i].mode & opts->mode)
                   || !match(&chopt, opthandlers[i].shortopt, &opthandlers[i].longopt))) {
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

    // `tag` must be post-processed if it does not yet have a value.
    if (opts->tag.len == 0) {
        opts->tag = opts->infile.len == 0
            ? strnew("stdin")
            : strrcut(&opts->infile, '/').tail;
    }

    // In `mask` mode, `append` and `prepend` are each populated with a single
    // magic value.
    if (opts->mode & OPTS_M_MASK) {
        handle_prepend(opts, &strnew("NONE"));
        handle_append(opts, &strnew("ANY"));
    }

    return true;
}

void optserr(options *opts, str *sbuf)
{
    opterrmsg msg = errmsg[opts->result];
    usize msglen = msg.fmt.len - (2 * msg.argc) + 1;
    if (msg.argc == 1) {
        msglen += opts->last_opt.len;
    } else if (msg.argc == 2) {
        msglen += opts->last_opt.len;
        msglen += opts->last_arg.len;
    }

    snprintf(sbuf->buf, msglen, msg.fmt.buf, opts->last_opt.buf, opts->last_arg.buf);
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

static bool handle_start_from(options *opts, str *arg)
{
    if (strtolong(arg, &opts->start)) {
        opts->set_start = true;
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
    opts->set_leader = true;
    opts->leader = strnewp(arg);
    return true;
}

static bool handle_tag_name(options *opts, str *arg)
{
    opts->set_tag = true;
    opts->tag = strnewp(arg);
    return true;
}

static bool handle_guard(options *opts, str *arg)
{
    opts->set_guard = true;
    opts->guard = strnewp(arg);
    return true;
}

static bool handle_lang(options *opts, str *arg)
{
    for (usize i = 0; generators[i].lang.len > 0; i++) {
        if (streq(&generators[i].lang, arg)) {
            opts->lang = strnewp(arg);
            opts->genf = i;
            return true;
        }
    }

    opts->result = OPTS_F_UNRECOGNIZED_LANG;
    return false;
}
