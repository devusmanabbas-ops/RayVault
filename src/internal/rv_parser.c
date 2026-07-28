#include "rv_parser.h"
#include "rv_log.h"

#include <string.h>

void rv_parser_init(rv_parser *p, rv_reader *r, rv_arena *a, rv_diag *d,
                    uint32_t flags)
{
    memset(p, 0, sizeof(*p));
    p->reader = r;
    p->arena = a;
    p->diag = d;
    p->flags = flags;
    p->result.phase = RV_PHASE_INIT;
}

void rv_parser_reset(rv_parser *p)
{
    rv_arena *a;
    rv_reader *r;
    rv_diag *d;
    uint32_t flags;
    if (!p)
        return;
    a = p->arena;
    r = p->reader;
    d = p->diag;
    flags = p->flags;
    memset(p, 0, sizeof(*p));
    p->arena = a;
    p->reader = r;
    p->diag = d;
    p->flags = flags;
    p->result.phase = RV_PHASE_INIT;
    if (a)
        rv_arena_reset(a);
}

const rv_parsed *rv_parser_result(const rv_parser *p)
{
    return p ? &p->result : NULL;
}

static int rv_parser_one_section(rv_parser *p, uint32_t tag, rv_parse_phase ph,
                                 int (*decode)(rv_arena *, rv_slice, void *),
                                 void *table)
{
    rv_dir_entry e;
    rv_slice sl;
    if (rv_reader_find_section(p->reader, tag, 0, &e) != 0) {
        /* optional sections are tolerated unless STRICT */
        if (p->flags & RV_OPEN_STRICT) {
            if (p->diag)
                rv_diag_add(p->diag, RV_ERR_SECTION, "missing required %s",
                            rv_tag_name(tag));
            p->result.phase = RV_PHASE_ERROR;
            return -1;
        }
        p->result.phase = ph;
        return 0;
    }
    if (!(p->flags & RV_OPEN_SKIP_SUMM) || tag != RV_TAG_SUMM) {
        if (rv_reader_verify_section_crc(p->reader, &e) != 0) {
            if (p->diag)
                rv_diag_add(p->diag, RV_ERR_CHECKSUM, "crc mismatch in %s",
                            rv_tag_name(tag));
            if (p->flags & RV_OPEN_STRICT) {
                p->result.phase = RV_PHASE_ERROR;
                return -1;
            }
        }
    }
    if (rv_reader_section_slice(p->reader, &e, &sl) != 0) {
        if (p->diag)
            rv_diag_add(p->diag, RV_ERR_OFFSET, "cannot map %s", rv_tag_name(tag));
        p->result.phase = RV_PHASE_ERROR;
        return -1;
    }
    if (decode(p->arena, sl, table) != 0) {
        if (p->diag)
            rv_diag_add(p->diag, RV_ERR_FORMAT, "decode failed for %s",
                        rv_tag_name(tag));
        p->result.phase = RV_PHASE_ERROR;
        return -1;
    }
    p->result.sections_seen++;
    p->result.phase = ph;
    return 0;
}

/* Adapters so we can use a common call shape. */
static int dec_snam(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_snam(a, s, (rv_snam_table *)t); }
static int dec_rout(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_rout(a, s, (rv_rout_table *)t); }
static int dec_inst(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_inst(a, s, (rv_inst_table *)t); }
static int dec_clbr(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_clbr(a, s, (rv_clbr_table *)t); }
static int dec_wind(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_wind(a, s, (rv_wind_table *)t); }
static int dec_wave(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_wave(a, s, (rv_wave_table *)t); }
static int dec_mark(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_mark(a, s, (rv_mark_table *)t); }
static int dec_link(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_link(a, s, (rv_link_table *)t); }
static int dec_note(rv_arena *a, rv_slice s, void *t)
{ return rv_decode_note(a, s, (rv_note_table *)t); }

static int rv_parser_decode_summ(rv_parser *p)
{
    rv_dir_entry e;
    rv_slice sl;
    if (rv_reader_find_section(p->reader, RV_TAG_SUMM, 0, &e) != 0) {
        p->result.phase = RV_PHASE_SUMM;
        return 0;
    }
    if (rv_reader_section_slice(p->reader, &e, &sl) != 0)
        return -1;
    if (rv_decode_summ(sl, &p->result.summ) != 0) {
        if (p->diag)
            rv_diag_add(p->diag, RV_ERR_FORMAT, "bad SUMM");
        return -1;
    }
    p->result.has_summ = 1;
    p->result.sections_seen++;
    p->result.phase = RV_PHASE_SUMM;
    return 0;
}

int rv_parser_step(rv_parser *p)
{
    int rc = 0;
    if (!p || !p->reader || !p->arena)
        return -1;

    switch (p->step_index) {
    case 0:
        p->result.phase = RV_PHASE_HEADER;
        p->result.feature_bits = p->reader->header.feature_bits;
        break;
    case 1:
        p->result.phase = RV_PHASE_DIR;
        break;
    case 2:
        rc = rv_parser_one_section(p, RV_TAG_SNAM, RV_PHASE_SNAM, dec_snam,
                                   &p->result.snam);
        break;
    case 3:
        rc = rv_parser_one_section(p, RV_TAG_ROUT, RV_PHASE_ROUT, dec_rout,
                                   &p->result.rout);
        break;
    case 4:
        rc = rv_parser_one_section(p, RV_TAG_INST, RV_PHASE_INST, dec_inst,
                                   &p->result.inst);
        break;
    case 5:
        rc = rv_parser_one_section(p, RV_TAG_CLBR, RV_PHASE_CLBR, dec_clbr,
                                   &p->result.clbr);
        break;
    case 6:
        rc = rv_parser_one_section(p, RV_TAG_WIND, RV_PHASE_WIND, dec_wind,
                                   &p->result.wind);
        break;
    case 7:
        rc = rv_parser_one_section(p, RV_TAG_WAVE, RV_PHASE_WAVE, dec_wave,
                                   &p->result.wave);
        break;
    case 8:
        rc = rv_parser_one_section(p, RV_TAG_MARK, RV_PHASE_MARK, dec_mark,
                                   &p->result.mark);
        break;
    case 9:
        rc = rv_parser_one_section(p, RV_TAG_LINK, RV_PHASE_LINK, dec_link,
                                   &p->result.link);
        break;
    case 10:
        rc = rv_parser_one_section(p, RV_TAG_NOTE, RV_PHASE_NOTE, dec_note,
                                   &p->result.note);
        break;
    case 11:
        rc = rv_parser_decode_summ(p);
        break;
    case 12:
        p->result.phase = RV_PHASE_DONE;
        return 1; /* finished */
    default:
        return 1;
    }
    if (rc != 0)
        return -1;
    p->step_index++;
    return 0;
}

int rv_parser_run(rv_parser *p)
{
    int rc;
    if (!p)
        return -1;
    p->step_index = 0;
    for (;;) {
        rc = rv_parser_step(p);
        if (rc < 0)
            return -1;
        if (rc > 0)
            return 0;
    }
}
