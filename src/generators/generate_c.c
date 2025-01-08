#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "generator.h"
#include "meta.h"
#include "options.h"
#include "strbuf.h"

typedef struct outlist {
    strlist *enums;
    strlist *procs;
    str *table;
} outlist;

static outlist *stringify(enumerator *input, const str *leader);
static int qsort_strcmp(const void *a, const void *b);
static str make_prefix(const str *prefix);
static str make_basename(const str *fname);

arena *local;

static str make_prefix(const str *prefix)
{
    char *buf = new (local, char, 1 << 8, A_F_ZERO | A_F_EXTEND);
    str cased = strsnake(prefix, buf, NULL, S_SNAKE_F_UPPER);
    cased.buf[cased.len] = '_';
    cased.buf[cased.len + 1] = '\0';
    cased.len++;

    return cased;
}

static str make_basename(const str *fname)
{
    char *buf = new (local, char, 1 << 8, A_F_ZERO | A_F_EXTEND);
    str fbase = strrcut(fname, '/').tail;
    return strsnake(&fbase, buf, &strnew("."), S_SNAKE_F_UPPER);
}

bool generate_c(enumerator *input, options *opts, FILE *fout)
{
    if (input == NULL || fout == NULL) {
        return false;
    }

    arena a = arena_new(1 << 16);
    local = &a;
    if (setjmp(local->env)) {
        free(local->mem);
        return false;
    }

    str leader = make_prefix(&opts->leader);
    str guardp = make_prefix(&opts->guard);
    str foutbn = make_basename(opts->outfile.len > 0 ? &opts->outfile : &strnew("stdout")); // TODO: Move this elsewhere...?

    outlist *genned = stringify(input, &leader);
    qsort(genned->table, input->count, sizeof(str), qsort_strcmp);

    fprintf(fout,
            "/*\n"
            " * %s\n"
            " */\n"
            "\n"
            "#ifndef %s%s\n"
            "#define %s%s\n"
            "\n"
            "#ifdef %sENUM\n"
            "\n"
            "enum %s {\n",
            header_warning.buf,
            guardp.buf, foutbn.buf,
            guardp.buf, foutbn.buf,
            guardp.buf,
            opts->tag.buf);

    strlist *e_curr = genned->enums;
    for (; e_curr; e_curr = e_curr->next) {
        fwrite(e_curr->elem.buf, 1, e_curr->elem.len, fout);
    }

    fprintf(fout,
            "};\n"
            "\n"
            "#else\n"
            "\n");

    strlist *p_curr = genned->procs;
    for (; p_curr; p_curr = p_curr->next) {
        fwrite(p_curr->elem.buf, 1, p_curr->elem.len, fout);
    }

    fprintf(fout,
            "\n"
            "#endif /* %sENUM */\n"
            "\n"
            "#ifdef %sLOOKUP\n"
            "\n"
            "struct %s_entry {\n"
            "    const long value;\n"
            "    const char *def;\n"
            "};\n"
            "\n"
            "#ifndef %sLOOKUP_IMPL\n"
            "\n"
            "extern const struct %s_entry %s_lookup[];\n"
            "\n"
            "#else\n"
            "\n"
            "const struct %s_entry %s_lookup[] = {\n",
            guardp.buf,
            guardp.buf,
            opts->tag.buf,
            guardp.buf,
            opts->tag.buf, opts->tag.buf,
            opts->tag.buf, opts->tag.buf);

    for (usize i = 0; i < input->count; i++) {
        fprintf(fout,
                "    { %.*s%s, \"%.*s\", },\n",
                (int)leader.len, leader.buf, genned->table[i].buf,
                (int)genned->table[i].len, genned->table[i].buf);
    }

    fprintf(fout,
            "};\n"
            "\n"
            "#endif /* %sLOOKUP_IMPL */\n"
            "\n"
            "#endif /* %sLOOKUP */\n"
            "\n"
            "#endif /* %s%s */\n",
            guardp.buf,
            guardp.buf,
            guardp.buf, foutbn.buf);

    free(local->mem);
    return true;
}

static inline char *stringify_entry(const str *leader, usize symbol_len, usize assign_len, usize maxlen, usize entry_len, isize assignment, const char *fmt)
{
    char *entry = new (local, char, entry_len + 1, A_F_ZERO | A_F_EXTEND);
    snprintf(entry,
             entry_len + 1,
             fmt,
             maxlen,
             (int)symbol_len, leader->buf,
             (int)assign_len, assignment);
    return entry;
}

static outlist *stringify(enumerator *input, const str *leader)
{
    usize maxlen = input->maxlen + leader->len;
    char *bufp = leader->buf + leader->len;
    enumerator *curr = input;

    outlist *outputs = new (local, outlist, 1, A_F_ZERO | A_F_EXTEND);
    outputs->enums = NULL;
    outputs->procs = NULL;
    outputs->table = new (local, str, input->count, A_F_ZERO | A_F_EXTEND);

    strlist **e_tail = &outputs->enums;
    strlist **p_tail = &outputs->procs;

    for (usize i = 0; curr; curr = curr->next, i++) {
        str cased_elem = strsnake(&curr->ident, bufp, NULL, S_SNAKE_F_UPPER);
        usize symbol_len = leader->len + cased_elem.len;

        char *claimed_elem = claim(local, cased_elem.buf, cased_elem.len + 1, A_F_ZERO | A_F_EXTEND);
        outputs->table[i] = strnew(claimed_elem, strlen(claimed_elem));

        usize assign_len = 6;

        // Format an enum entry: '    ' -> symbol -> ' = ' -> assignment + ',\n'
        usize enum_entry_len = maxlen + assign_len + 9;
        char *enum_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           maxlen,
                                           enum_entry_len,
                                           curr->assignment,
                                           "    %-*.*s = %*ld,\n");
        strlist_append(e_tail, local, strnew(enum_entry, enum_entry_len), A_F_EXTEND);

        // Format a preproc entry: '#define ' -> symbol -> ' ' -> assignment + '\n'
        usize proc_entry_len = maxlen + assign_len + 10;
        char *proc_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           maxlen,
                                           proc_entry_len,
                                           curr->assignment,
                                           "#define %-*.*s %*ld\n");
        strlist_append(p_tail, local, strnew(proc_entry, proc_entry_len), A_F_EXTEND);
    }

    return outputs;
}

static int qsort_strcmp(const void *a, const void *b)
{
    const str *s1 = a;
    const str *s2 = b;
    return strcmp(s1->buf, s2->buf);
}
