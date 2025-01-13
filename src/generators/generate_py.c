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

static arena *local;

// clang-format off
static const char *enum_fmt = "    %-*.*s = %*ld\n";
static const char *mask_fmt = "    %-*.*s =  (1 << %*ld)\n";
static const char *mask_fmt_0 = "    %-*.*s =        %*ld\n";
static const char *mask_fmt_l = "    %-*.*s = ((1 << %*ld) - 1)\n";

static const char *header_fmt = ""
    "\"\"\"\n"
    "    %s\n"
    "    Base command: %s\n"
    "    Source file: %s\n"
    "    Program options:\n"
    "";

static const char *imports_fmt = ""
    "\"\"\"\n"
    "\n"
    "import enum\n"
    "\n"
    "class %s(enum.%s):\n"
    "";
// clang-format on

bool generate_py(enumerator *input, options *opts, FILE *fout)
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
    outlist *genned = stringify(input, &leader, opts->mode);

    fprintf(fout, header_fmt,
            header_warning.buf,
            opts->mode & OPTS_M_ENUM ? "enum" : "mask",
            opts->infile.buf);

    write_options(opts, fout);

    fprintf(fout, imports_fmt,
            opts->tag.buf,
            opts->mode & OPTS_M_ENUM ? "IntEnum" : "IntFlag");

    strlist *e_curr = genned->enums;
    for (; e_curr; e_curr = e_curr->next) {
        fwrite(e_curr->elem.buf, 1, e_curr->elem.len, fout);
    }

    free(local->mem);
    return true;
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

static void stringify_enumeration(enumerator *input, const str *leader, outlist *outputs, usize max_ident_len)
{
    usize enum_entry_len_base = max_ident_len + 8;
    usize assign_len = input->max_assign_len;
    char *bufp = leader->buf + leader->len;

    strlist **e_tail = &outputs->enums;
    enumerator *curr = input;

    for (usize i = 0; curr; curr = curr->next, i++) {
        str cased_elem = strsnake(&curr->ident, bufp, NULL, S_SNAKE_F_UPPER);
        usize symbol_len = leader->len + cased_elem.len;

        char *claimed_elem = claim(local, cased_elem.buf, cased_elem.len + 1, A_F_ZERO | A_F_EXTEND);
        outputs->table[i] = strnew(claimed_elem, strlen(claimed_elem));

        // Format an enum entry: '    ' -> symbol -> ' = ' -> assignment + '\n'
        char *enum_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           max_ident_len,
                                           enum_entry_len_base + assign_len,
                                           curr->assignment,
                                           enum_fmt);
        strlist_append(e_tail, local, strnew(enum_entry, strlen(enum_entry)), A_F_EXTEND);
    }
}

static void stringify_bitmask(enumerator *input, const str *leader, outlist *outputs, usize max_ident_len)
{
    usize enum_entry_len_base = max_ident_len + 16;
    usize assign_len = input->max_assign_len;
    char *bufp = leader->buf + leader->len;

    strlist **e_tail = &outputs->enums;
    enumerator *curr = input;

    for (usize i = 0; curr; curr = curr->next, i++) {
        str cased_elem = strsnake(&curr->ident, bufp, NULL, S_SNAKE_F_UPPER);
        usize symbol_len = leader->len + cased_elem.len;

        char *claimed_elem = claim(local, cased_elem.buf, cased_elem.len + 1, A_F_ZERO | A_F_EXTEND);
        outputs->table[i] = strnew(claimed_elem, strlen(claimed_elem));

        const char *e_fmt = mask_fmt;
        usize assignment = curr->assignment;
        if (i == 0) { // first element
            e_fmt = mask_fmt_0;
            assignment = 1;              // gets set to 0 by the subtraction below
        } else if (curr->next == NULL) { // last element
            e_fmt = mask_fmt_l;
            enum_entry_len_base += 5;
        }

        // Format an enum entry: '    ' -> symbol -> ' = ' -> assignment + '\n'
        char *enum_entry = stringify_entry(leader,
                                           symbol_len,
                                           assign_len,
                                           max_ident_len,
                                           enum_entry_len_base + assign_len,
                                           assignment - 1,
                                           e_fmt);
        strlist_append(e_tail, local, strnew(enum_entry, strlen(enum_entry)), A_F_EXTEND);
    }
}

static void write_options(options *opts, FILE *fout)
{
    fprintf(fout, "%s\n", "      --lang py");

    if (opts->set_leader) {
        fprintf(fout, "      --leader %s\n", opts->leader.buf);
    }

    if (opts->set_tag) {
        fprintf(fout, "      --tag-name %s\n", opts->tag.buf);
    }

    if (opts->set_guard) {
        fprintf(fout, "      --guard %s\n", opts->guard.buf);
    }

    if (!(opts->mode & OPTS_M_MASK)) {
        for (usize i = 0; i < opts->append_count; i++) {
            fprintf(fout, "      --append %s\n", opts->append[i].buf);
        }

        for (usize i = 0; i < opts->prepend_count; i++) {
            fprintf(fout, " *    --prepend %s\n", opts->prepend[i].buf);
        }

        if (opts->set_start) {
            fprintf(fout, " *    --start-from %ld\n", opts->start);
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
