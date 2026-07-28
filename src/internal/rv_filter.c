#include "rv_filter.h"

#include <string.h>

void rv_span_filter_init(rv_span_filter *f)
{
    if (f)
        memset(f, 0, sizeof(*f));
}

int rv_span_filter_match_window(const rv_parsed *p, const rv_window_rec *w,
                                const rv_span_filter *f)
{
    uint32_t i;
    if (!p || !w || !f)
        return 0;
    if (f->route_id && w->route_id != f->route_id)
        return 0;
    if (f->inst_id && w->inst_id != f->inst_id)
        return 0;
    if (f->acquired_after && w->acquired_unix < f->acquired_after)
        return 0;
    if (f->acquired_before && w->acquired_unix > f->acquired_before)
        return 0;
    if (f->min_range_km > 0.0f && w->range_km < f->min_range_km)
        return 0;
    if (f->max_range_km > 0.0f && w->range_km > f->max_range_km)
        return 0;
    if (f->wavelength_nm) {
        int found = 0;
        for (i = 0; i < p->inst.count; i++) {
            if (p->inst.items[i].inst_id == w->inst_id) {
                if (p->inst.items[i].wavelength_nm == f->wavelength_nm)
                    found = 1;
                break;
            }
        }
        if (!found)
            return 0;
    }
    return 1;
}

int rv_span_filter_windows(rv_arena *a, const rv_parsed *p, const rv_index *idx,
                           const rv_span_filter *f, rv_filtered_windows *out)
{
    uint32_t i, n = 0;
    uint32_t *ids;
    if (!a || !p || !idx || !f || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    for (i = 0; i < p->wind.count; i++) {
        if (rv_span_filter_match_window(p, &p->wind.items[i], f))
            n++;
    }
    if (n == 0)
        return 0;
    ids = (uint32_t *)rv_arena_calloc(a, n, sizeof(uint32_t), 4);
    if (!ids)
        return -1;
    n = 0;
    for (i = 0; i < p->wind.count; i++) {
        if (rv_span_filter_match_window(p, &p->wind.items[i], f))
            ids[n++] = p->wind.items[i].window_id;
    }
    out->ids = ids;
    out->count = n;
    (void)idx;
    return 0;
}

int rv_span_filter_apply_to_markers(const rv_index *idx,
                                    const rv_filtered_windows *wins,
                                    rv_marker_info *out, size_t cap,
                                    size_t *written)
{
    size_t n = 0;
    uint32_t i, j;
    if (!idx || !wins || !written)
        return -1;
    for (i = 0; i < idx->marker_count; i++) {
        int ok = 0;
        for (j = 0; j < wins->count; j++) {
            if (idx->markers[i].window_id == wins->ids[j]) {
                ok = 1;
                break;
            }
        }
        if (!ok)
            continue;
        if (out && n < cap) {
            out[n].marker_id = idx->markers[i].marker_id;
            out[n].window_id = idx->markers[i].window_id;
            out[n].distance_m = idx->markers[i].distance_m;
            out[n].kind = idx->markers[i].kind;
            out[n].severity = idx->markers[i].severity;
            out[n].label_name_id =
                idx->markers[i].rec ? idx->markers[i].rec->label_name_id : 0;
        }
        n++;
    }
    *written = n;
    return 0;
}
