#include "rv_xref.h"
#include "rv_dict.h"

#include <string.h>

static int rv_find_inst(const rv_parsed *p, uint32_t id, uint32_t *pos)
{
    uint32_t i;
    for (i = 0; i < p->inst.count; i++) {
        if (p->inst.items[i].inst_id == id) {
            if (pos)
                *pos = i;
            return 0;
        }
    }
    return -1;
}

static int rv_find_calib(const rv_parsed *p, uint32_t id, uint32_t *pos)
{
    uint32_t i;
    for (i = 0; i < p->clbr.count; i++) {
        if (p->clbr.items[i].calib_id == id) {
            if (pos)
                *pos = i;
            return 0;
        }
    }
    return -1;
}

int rv_xref_resolve_wave_for_window(const rv_parsed *p, const rv_index *idx,
                                    uint32_t window_id, uint32_t *wave_pos)
{
    uint32_t wpos, block_id;
    if (!p || !idx || !wave_pos)
        return -1;
    if (rv_index_find_window(idx, window_id, &wpos) != 0)
        return -1;
    block_id = idx->windows[wpos].rec->wave_block_id;
    return rv_index_find_wave(idx, block_id, wave_pos);
}

int rv_xref_resolve_calib_for_inst(const rv_parsed *p, uint32_t inst_id,
                                   uint32_t *calib_pos)
{
    uint32_t i;
    if (!p || !calib_pos)
        return -1;
    for (i = 0; i < p->clbr.count; i++) {
        if (p->clbr.items[i].inst_id == inst_id) {
            *calib_pos = i;
            return 0;
        }
    }
    return -1;
}

int rv_xref_validate(const rv_parsed *p, const rv_index *idx,
                     rv_diag *diag, rv_xref_report *rep)
{
    uint32_t i;
    rv_xref_report local;
    memset(&local, 0, sizeof(local));
    if (!p || !idx)
        return -1;

    for (i = 0; i < p->wind.count; i++) {
        const rv_window_rec *w = &p->wind.items[i];
        uint32_t pos;
        local.checked++;
        if (rv_index_find_route(idx, w->route_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "window %u route %u missing", w->window_id,
                            w->route_id);
        }
        if (rv_find_inst(p, w->inst_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "window %u instrument %u missing", w->window_id,
                            w->inst_id);
        }
        if (w->calib_id && rv_find_calib(p, w->calib_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "window %u calib %u missing", w->window_id,
                            w->calib_id);
        }
        if (w->wave_block_id &&
            rv_index_find_wave(idx, w->wave_block_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "window %u wave %u missing", w->window_id,
                            w->wave_block_id);
        }
    }

    for (i = 0; i < p->mark.count; i++) {
        const rv_marker_rec *m = &p->mark.items[i];
        uint32_t pos;
        local.checked++;
        if (rv_index_find_window(idx, m->window_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "marker %u window %u missing", m->marker_id,
                            m->window_id);
        }
    }

    for (i = 0; i < p->wave.count; i++) {
        const rv_wave_block *b = &p->wave.items[i];
        uint32_t pos;
        local.checked++;
        if (b->window_id && rv_index_find_window(idx, b->window_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "wave %u window %u missing", b->block_id,
                            b->window_id);
        }
    }

    for (i = 0; i < p->link.count; i++) {
        const rv_link_rec *l = &p->link.items[i];
        uint32_t pos = 0;
        local.checked++;
        if (l->from_kind == 1 &&
            rv_index_find_route(idx, l->from_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "link %u from route %u missing", l->link_id,
                            l->from_id);
        }
        if (l->to_kind == 2 &&
            rv_index_find_window(idx, l->to_id, &pos) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF,
                            "link %u to window %u missing", l->link_id,
                            l->to_id);
        }
    }

    {
        uint32_t bad = 0;
        if (rv_dict_check_routes(&p->snam, &p->rout, &bad) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF, "route name id %u missing",
                            bad);
        }
        if (rv_dict_check_instruments(&p->snam, &p->inst, &bad) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF, "inst name id %u missing",
                            bad);
        }
        if (rv_dict_check_markers(&p->snam, &p->mark, &bad) != 0) {
            local.failures++;
            if (diag)
                rv_diag_add(diag, RV_ERR_CROSSREF, "marker name id %u missing",
                            bad);
        }
    }

    if (rep)
        *rep = local;
    return local.failures ? -1 : 0;
}
