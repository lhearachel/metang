#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "generator.h"
#include "meta.h"
#include "options.h"
#include "strbuf.h"

static outlist *stringify(enumerator *input, const str *leader, enum options_mode mode);
static void stringify_enumeration(enumerator *input, const str *leader, outlist *outputs, usize max_ident_len);
static void stringify_bitmask(enumerator *input, const str *leader, outlist *outputs, usize max_ident_len);

static void write_options(options *opts, FILE *fout);
static str make_prefix(const str *prefix);
static str make_basename(const str *fname);
static int qsort_strcmp(const void *a, const void *b);

static arena *local;

// clang-format off
static const char *e_enum_fmt = "    %-*.*s = %*ld,\n";
static const char *p_enum_fmt = "#define %-*.*s %*ld\n";
static const char *e_mask_fmt = "    %-*.*s =  (1 << %*ld),\n";
static const char *p_mask_fmt = "#define %-*.*s  (1 << %*ld)\n";
static const char *e_mask_fmt_0 = "    %-*.*s =        %*ld,\n";
static const char *e_mask_fmt_l = "    %-*.*s = ((1 << %*ld) - 1),\n";
static const char *p_mask_fmt_0 = "#define %-*.*s        %*ld\n";
static const char *p_mask_fmt_l = "#define %-*.*s ((1 << %*ld) - 1)\n";

static const char *header_fmt = ""
    "/*\n"
    " * %s\n"
    " * Base command: %s\n"
    " * Source file: %s\n"
    " * Program options:\n"
    "";

static const char *init_guards_fmt = ""
    " */\n"
    "\n"
    "#ifndef %s%s\n"
    "#define %s%s\n"
    "\n"
    "#ifdef __cplusplus\n"
    "extern \"C\" {\n"
    "#endif\n"
    "\n"
    "#ifdef %sENUM\n"
    "\n"
    "enum %s {\n"
    "";

static const char *lookup_branch_fmt = ""
    "\n"
    "#endif /* %sENUM */\n"
    "\n"
    "#ifdef %sLOOKUP\n"
    "\n"
    "typedef struct entry__%s {\n"
    "    const long value;\n"
    "    const char *def;\n"
    "} entry__%s;\n"
    "\n"
    "#ifndef %sLOOKUP_IMPL\n"
    "\n"
    "extern const long lengthof__%s;\n"
    "extern const entry__%s lookup__%s[];\n"
    "\n"
    "#else\n"
    "\n"
    "const long lengthof__%s = %d;\n"
    "const entry__%s lookup__%s[] = {\n"
    "";

static const char *footer_fmt = ""
    "};\n"
    "\n"
    "#endif /* %sLOOKUP_IMPL */\n"
    "\n"
    "#endif /* %sLOOKUP */\n"
    "\n"
    "#ifdef __cplusplus\n"
    "}\n"
    "#endif\n"
    "\n"
    "#endif /* %s%s */\n"
    "";
// clang-format on

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
    str foutbn = make_basename(&opts->outfile);

    outlist *genned = stringify(input, &leader, opts->mode);
    qsort(genned->table, input->count, sizeof(str), qsort_strcmp);

    fprintf(fout, header_fmt,
            header_warning.buf,
            opts->mode & OPTS_M_ENUM ? "enum" : "mask",
            opts->infile.buf);

    write_options(opts, fout);

    fprintf(fout, init_guards_fmt,
            guardp.buf, foutbn.buf,
            guardp.buf, foutbn.buf,
            guardp.buf,
            opts->tag.buf);

    strlist *e_curr = genned->enums;
    for (; e_curr; e_curr = e_curr->next) {
        fwrite(e_curr->elem.buf, 1, e_curr->elem.len, fout);
    }

    fprintf(fout, "};\n\n#else\n\n");

    strlist *p_curr = genned->procs;
    for (; p_curr; p_curr = p_curr->next) {
        fwrite(p_curr->elem.buf, 1, p_curr->elem.len, fout);
    }

    fprintf(fout, lookup_branch_fmt,
            guardp.buf,                    // "#endif /* %sENUM */\n"
            guardp.buf,                    // "#ifdef %sLOOKUP\n"
            opts->tag.buf,                 // "typedef struct entry__%s {\n"
            opts->tag.buf,                 // "} entry__%s;\n"
            guardp.buf,                    // "#ifndef %sLOOKUP_IMPL\n"
            opts->tag.buf,                 // "extern const long lengthof__%s;\n"
            opts->tag.buf, opts->tag.buf,  // "extern const entry__%s lookup__%s[];\n"
            opts->tag.buf, input->count,   // "const long lengthof__%s = %d;\n"
            opts->tag.buf, opts->tag.buf); // "const entry__%s lookup__%s[] = {\n"

    for (usize i = 0; i < input->count; i++) {
        usize padding = input->max_ident_len - genned->table[i].len + 1;
        fprintf(fout,
                "    { %.*s%s,%*c\"%.*s%s\",%*c},\n",
                (int)leader.len, leader.buf, genned->table[i].buf,
                (int)padding, ' ',
                (int)leader.len, leader.buf, genned->table[i].buf,
                (int)padding, ' ');
    }

    fprintf(fout, footer_fmt,
            guardp.buf,
            guardp.buf,
            guardp.buf, foutbn.buf);

    free(local->mem);
    return true;
}

static inline char *stringify_entry(
    const str *leader,
    usize symbol_len,
    usize assign_len,
    usize column_len,
    usize entry_len,
    isize assignment,
    const char *fmt)
{
    char *entry = new (local, char, entry_len + 1, A_F_ZERO | A_F_EXTEND);
    snprintf(entry,
             entry_len + 1,
             fmt,
             column_len,
             (int)symbol_len, leader->buf,
             (int)assign_len, assignment);
    return entry;
}

static outlist *stringify(enumerator *input, const str *leader, enum options_mode mode)
{
    outlist *outputs = new (local, outlist, 1, A_F_ZERO | A_F_EXTEND);
    outputs->enums = NULL;
    outputs->procs = NULL;
    outputs->table = new (local, str, input->count, A_F_ZERO | A_F_EXTEND);

    if (mode == OPTS_M_ENUM) {
        stringify_enumeration(input, leader, outputs, input->max_ident_len + leader->len);
    } else {
        stringify_bitmask(input, leader, outputs, input->max_ident_len + leader->len);
    }

    return outputs;
}

static void stringify_enumeration(enumerator *input, const str *leader, outlist *outputs, usize max_ident_len)
{
    usize enum_entry_len_base = max_ident_len + 9;
    usize proc_entry_len_base = max_ident_len + 10;
    usize assign_len = input->max_assign_len;
    char *bufp = leader->buf + leader->len;

    strlist **e_tail = &outputs->enums;
    strlist **p_tail = &outputs->procs;
    enumerator *curr = input;

    for (usize i = 0; curr; curr = curr->next, i++) {
        str cased_elem = strsnake(&curr->ident, bufp, NULL, S_SNAKE_F_UPPER);
        usize symbol_len = leader->len + cased_elem.len;

        char *claimed_elem = claim(local, cased_elem.buf, cased_elem.len + 1, A_F_ZERO | A_F_EXTEND);
        outputs->table[i] = strnew(claimed_elem, strlen(claimed_elem));

        // Format an enum entry: '    ' -> symbol -> ' = ' -> assignment + ',\n'
        char *enum_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           max_ident_len,
                                           enum_entry_len_base + assign_len,
                                           curr->assignment,
                                           e_enum_fmt);
        strlist_append(e_tail, local, strnew(enum_entry, strlen(enum_entry)), A_F_EXTEND);

        // Format a preproc entry: '#define ' -> symbol -> ' ' -> assignment + '\n'
        char *proc_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           max_ident_len,
                                           proc_entry_len_base + assign_len,
                                           curr->assignment,
                                           p_enum_fmt);
        strlist_append(p_tail, local, strnew(proc_entry, strlen(proc_entry)), A_F_EXTEND);
    }
}

static void stringify_bitmask(enumerator *input, const str *leader, outlist *outputs, usize max_ident_len)
{
    usize enum_entry_len_base = max_ident_len + 17;
    usize proc_entry_len_base = max_ident_len + 18;
    usize assign_len = input->max_assign_len;
    char *bufp = leader->buf + leader->len;

    strlist **e_tail = &outputs->enums;
    strlist **p_tail = &outputs->procs;
    enumerator *curr = input;

    for (usize i = 0; curr; curr = curr->next, i++) {
        str cased_elem = strsnake(&curr->ident, bufp, NULL, S_SNAKE_F_UPPER);
        usize symbol_len = leader->len + cased_elem.len;

        char *claimed_elem = claim(local, cased_elem.buf, cased_elem.len + 1, A_F_ZERO | A_F_EXTEND);
        outputs->table[i] = strnew(claimed_elem, strlen(claimed_elem));

        const char *e_fmt = e_mask_fmt;
        const char *p_fmt = p_mask_fmt;
        usize assignment = curr->assignment;
        if (i == 0) { // first element
            e_fmt = e_mask_fmt_0;
            p_fmt = p_mask_fmt_0;
            assignment = 1;              // gets set to 0 by the subtraction below
        } else if (curr->next == NULL) { // last element
            e_fmt = e_mask_fmt_l;
            p_fmt = p_mask_fmt_l;
            enum_entry_len_base += 5;
            proc_entry_len_base += 5;
        }

        // Format an enum entry: '    ' -> symbol -> ' = ' -> assignment + ',\n'
        char *enum_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           max_ident_len,
                                           enum_entry_len_base + assign_len,
                                           assignment - 1,
                                           e_fmt);
        strlist_append(e_tail, local, strnew(enum_entry, strlen(enum_entry)), A_F_EXTEND);

        // Format a preproc entry: '#define ' -> symbol -> ' ' -> assignment + '\n'
        char *proc_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           max_ident_len,
                                           proc_entry_len_base + assign_len,
                                           assignment - 1,
                                           p_fmt);
        strlist_append(p_tail, local, strnew(proc_entry, strlen(proc_entry)), A_F_EXTEND);
    }
}

static void write_options(options *opts, FILE *fout)
{
    if (opts->set_leader) {
        fprintf(fout, " *   --leader %s\n", opts->leader.buf);
    }

    if (opts->set_tag) {
        fprintf(fout, " *   --tag-name %s\n", opts->tag.buf);
    }

    if (opts->set_guard) {
        fprintf(fout, " *   --guard %s\n", opts->guard.buf);
    }

    if (!(opts->mode & OPTS_M_MASK)) {
        for (usize i = 0; i < opts->append_count; i++) {
            fprintf(fout, " *   --append %s\n", opts->append[i].buf);
        }

        for (usize i = 0; i < opts->prepend_count; i++) {
            fprintf(fout, " *   --prepend %s\n", opts->prepend[i].buf);
        }

        if (opts->set_start) {
            fprintf(fout, " *   --start-from %ld\n", opts->start);
        }
    }
}

static str make_prefix(const str *prefix)
{
    if (prefix->len == 0) {
        char *buf = new (local, char, 1 << 8, A_F_ZERO | A_F_EXTEND);
        return strnew(buf, 0);
    }

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

static int qsort_strcmp(const void *a, const void *b)
{
    const str *s1 = a;
    const str *s2 = b;
    return strcmp(s1->buf, s2->buf);
}
