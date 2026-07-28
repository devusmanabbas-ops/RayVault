#include "rv_query.h"

#include <stdlib.h>
#include <string.h>

void rv_query_plan_init(rv_query_plan *plan)
{
    if (!plan)
        return;
    memset(plan, 0, sizeof(*plan));
    plan->limit = 1024;
}

static int rv_marker_info_cmp_dist(const void *a, const void *b)
{
    const rv_marker_info *ma = (const rv_marker_info *)a;
    const rv_marker_info *mb = (const rv_marker_info *)b;
    if (ma->distance_m < mb->distance_m)
        return -1;
    if (ma->distance_m > mb->distance_m)
        return 1;
    return 0;
}

static int rv_marker_info_cmp_sev(const void *a, const void *b)
{
    const rv_marker_info *ma = (const rv_marker_info *)a;
    const rv_marker_info *mb = (const rv_marker_info *)b;
    if (ma->severity > mb->severity)
        return -1;
    if (ma->severity < mb->severity)
        return 1;
    return 0;
}

int rv_query_windows_for_route(const rv_index *idx, uint32_t route_id,
                               uint32_t *out, size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i;
    if (!idx || !written)
        return -1;
    for (i = 0; i < idx->window_count; i++) {
        if (route_id && idx->windows[i].route_id != route_id)
            continue;
        if (out && n < cap)
            out[n] = idx->windows[i].window_id;
        n++;
    }
    *written = n;
    return 0;
}

int rv_query_markers_near(const rv_index *idx, uint32_t window_id,
                          float center_m, float radius_m,
                          rv_marker_info *out, size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i;
    if (!idx || !written)
        return -1;
    for (i = 0; i < idx->marker_count; i++) {
        const rv_marker_index_node *m = &idx->markers[i];
        float d;
        if (window_id && m->window_id != window_id)
            continue;
        d = m->distance_m - center_m;
        if (d < 0)
            d = -d;
        if (d > radius_m)
            continue;
        if (out && n < cap) {
            out[n].marker_id = m->marker_id;
            out[n].window_id = m->window_id;
            out[n].distance_m = m->distance_m;
            out[n].kind = m->kind;
            out[n].severity = m->severity;
            out[n].label_name_id = m->rec ? m->rec->label_name_id : 0;
        }
        n++;
    }
    *written = n;
    return 0;
}

int rv_query_instruments_on_route(const rv_parsed *p, const rv_index *idx,
                                  uint32_t route_id, uint32_t *out,
                                  size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i, j;
    if (!p || !idx || !written)
        return -1;
    for (i = 0; i < idx->window_count; i++) {
        uint32_t inst;
        int seen = 0;
        if (route_id && idx->windows[i].route_id != route_id)
            continue;
        inst = idx->windows[i].inst_id;
        for (j = 0; j < n; j++) {
            if (out && out[j] == inst) {
                seen = 1;
                break;
            }
        }
        if (seen)
            continue;
        if (out && n < cap)
            out[n] = inst;
        n++;
    }
    *written = n;
    (void)p;
    return 0;
}

int rv_query_execute(rv_arena *a, const rv_parsed *p, const rv_index *idx,
                     const rv_query_plan *plan, rv_query_result *out)
{
    size_t cap, written = 0;
    rv_marker_info *tmp;
    size_t wcap, wwritten = 0;
    if (!a || !p || !idx || !plan || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    cap = plan->limit ? plan->limit : 1024;
    tmp = (rv_marker_info *)rv_arena_calloc(a, cap, sizeof(*tmp), 8);
    if (!tmp)
        return -1;
    if (rv_index_query_markers(idx, &plan->filter, tmp, cap, &written) != 0)
        return -1;
    if (written > cap)
        written = cap;
    if (plan->require_wave) {
        size_t i, w = 0;
        for (i = 0; i < written; i++) {
            uint32_t wpos, wave_pos;
            if (rv_index_find_window(idx, tmp[i].window_id, &wpos) != 0)
                continue;
            if (rv_index_find_wave(idx, idx->windows[wpos].rec->wave_block_id,
                                   &wave_pos) != 0)
                continue;
            if (w != i)
                tmp[w] = tmp[i];
            w++;
        }
        written = w;
    }
    if (plan->sort_by_distance && written > 1)
        qsort(tmp, written, sizeof(*tmp), rv_marker_info_cmp_dist);
    else if (plan->sort_by_severity && written > 1)
        qsort(tmp, written, sizeof(*tmp), rv_marker_info_cmp_sev);

    out->markers = tmp;
    out->count = written;

    wcap = idx->window_count ? idx->window_count : 1;
    out->window_ids = (uint32_t *)rv_arena_calloc(a, wcap, sizeof(uint32_t), 4);
    if (!out->window_ids)
        return -1;
    (void)rv_query_windows_for_route(idx, plan->filter.route_id,
                                     out->window_ids, wcap, &wwritten);
    out->window_count = wwritten;
    return 0;
}
