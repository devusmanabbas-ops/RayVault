#include "rv_validate.h"
#include "rv_dict.h"
#include "rv_xref.h"

#include <math.h>
#include <string.h>

void rv_validate_options_default(rv_validate_options *o)
{
    if (!o)
        return;
    memset(o, 0, sizeof(*o));
    o->check_crc = 1;
    o->check_xref = 1;
    o->check_summ = 1;
    o->check_ranges = 1;
    o->check_names = 1;
}

int rv_validate_header_consistency(const rv_reader *r, rv_diag *diag)
{
    if (!r)
        return -1;
    if (r->header.total_size != 0 &&
        r->header.total_size != r->backing.size) {
        if (diag)
            rv_diag_add(diag, RV_ERR_FORMAT,
                        "header total_size %llu != file %zu",
                        (unsigned long long)r->header.total_size,
                        r->backing.size);
        return -1;
    }
    if (r->header.section_count != r->dir_count) {
        if (diag)
            rv_diag_add(diag, RV_ERR_SECTION, "section_count mismatch");
        return -1;
    }
    return 0;
}

int rv_validate_summ_counts(const rv_parsed *p, rv_diag *diag)
{
    if (!p || !p->has_summ)
        return 0;
    if (p->summ.route_count && p->summ.route_count != p->rout.count) {
        if (diag)
            rv_diag_add(diag, RV_ERR_FORMAT, "SUMM route_count mismatch");
        return -1;
    }
    if (p->summ.window_count && p->summ.window_count != p->wind.count) {
        if (diag)
            rv_diag_add(diag, RV_ERR_FORMAT, "SUMM window_count mismatch");
        return -1;
    }
    if (p->summ.marker_count && p->summ.marker_count != p->mark.count) {
        if (diag)
            rv_diag_add(diag, RV_ERR_FORMAT, "SUMM marker_count mismatch");
        return -1;
    }
    if (p->summ.wave_count && p->summ.wave_count != p->wave.count) {
        if (diag)
            rv_diag_add(diag, RV_ERR_FORMAT, "SUMM wave_count mismatch");
        return -1;
    }
    if (p->summ.name_count && p->summ.name_count != p->snam.count) {
        if (diag)
            rv_diag_add(diag, RV_ERR_FORMAT, "SUMM name_count mismatch");
        return -1;
    }
    return 0;
}

int rv_validate_physical_ranges(const rv_parsed *p, rv_diag *diag)
{
    uint32_t i;
    if (!p)
        return -1;
    for (i = 0; i < p->rout.count; i++) {
        if (p->rout.items[i].length_km < 0.0f ||
            p->rout.items[i].length_km > 500.0f) {
            if (diag)
                rv_diag_add(diag, RV_ERR_FORMAT, "route %u length out of range",
                            p->rout.items[i].route_id);
            return -1;
        }
    }
    for (i = 0; i < p->wind.count; i++) {
        if (p->wind.items[i].pulse_ns < 0.0f ||
            p->wind.items[i].range_km < 0.0f) {
            if (diag)
                rv_diag_add(diag, RV_ERR_FORMAT, "window %u bad ranges",
                            p->wind.items[i].window_id);
            return -1;
        }
    }
    for (i = 0; i < p->wave.count; i++) {
        if (p->wave.items[i].step_m <= 0.0f) {
            if (diag)
                rv_diag_add(diag, RV_ERR_FORMAT, "wave %u non-positive step",
                            p->wave.items[i].block_id);
            return -1;
        }
    }
    for (i = 0; i < p->mark.count; i++) {
        if (p->mark.items[i].distance_m < 0.0f) {
            if (diag)
                rv_diag_add(diag, RV_ERR_FORMAT, "marker %u negative distance",
                            p->mark.items[i].marker_id);
            return -1;
        }
    }
    return 0;
}

int rv_validate_package(const rv_reader *r, const rv_parsed *p,
                        const rv_index *idx, const rv_validate_options *o,
                        rv_diag *diag)
{
    rv_validate_options def;
    int rc = 0;
    if (!o) {
        rv_validate_options_default(&def);
        o = &def;
    }
    if (!r || !p)
        return -1;

    if (rv_validate_header_consistency(r, diag) != 0)
        rc = -1;

    if (o->check_crc) {
        uint32_t i;
        for (i = 0; i < r->dir_count; i++) {
            if (rv_reader_verify_section_crc(r, &r->dir[i]) != 0) {
                if (diag)
                    rv_diag_add(diag, RV_ERR_CHECKSUM, "section %u crc", i);
                rc = -1;
            }
        }
    }
    if (o->check_names) {
        uint32_t bad = 0;
        if (rv_dict_check_routes(&p->snam, &p->rout, &bad) != 0 ||
            rv_dict_check_instruments(&p->snam, &p->inst, &bad) != 0 ||
            rv_dict_check_markers(&p->snam, &p->mark, &bad) != 0)
            rc = -1;
    }
    if (o->check_xref && idx && idx->ready) {
        rv_xref_report rep;
        if (rv_xref_validate(p, idx, diag, &rep) != 0)
            rc = -1;
    }
    if (o->check_summ && rv_validate_summ_counts(p, diag) != 0)
        rc = -1;
    if (o->check_ranges && rv_validate_physical_ranges(p, diag) != 0)
        rc = -1;
    return rc;
}
