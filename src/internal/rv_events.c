#include "rv_events.h"

#include <stdlib.h>
#include <string.h>

const char *rv_event_kind_name(uint16_t kind)
{
    switch (kind) {
    case RV_EVT_SPLICE:  return "splice";
    case RV_EVT_REFLECT: return "reflectance";
    case RV_EVT_OPEN:    return "fiber-break";
    case RV_EVT_BEND:    return "macrobend";
    case RV_EVT_GAIN:    return "gain";
    case RV_EVT_END:     return "end-of-fiber";
    case RV_EVT_GHOST:   return "ghost";
    default:             return "unknown";
    }
}

int rv_event_classify_marker(const rv_marker_index_node *m, rv_event_class *out)
{
    float loss, refl;
    if (!m || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    out->marker_id = m->marker_id;
    out->kind = m->kind;
    out->severity = m->severity;
    out->distance_m = m->distance_m;
    out->label = m->label;
    loss = m->rec ? m->rec->loss_db : 0.0f;
    refl = m->rec ? m->rec->reflectance_db : 0.0f;
    out->loss_db = loss;
    out->reflectance_db = refl;

    out->is_actionable = 0;
    out->is_ghost_candidate = 0;

    if (m->kind == RV_EVT_OPEN || m->severity >= 80)
        out->is_actionable = 1;
    if (m->kind == RV_EVT_SPLICE && loss > 0.5f)
        out->is_actionable = 1;
    if (m->kind == RV_EVT_REFLECT && refl > -40.0f)
        out->is_actionable = 1;
    if (m->kind == RV_EVT_GHOST || (m->kind == RV_EVT_REFLECT && loss < 0.05f &&
                                    refl < -60.0f))
        out->is_ghost_candidate = 1;

    return 0;
}

int rv_event_classify_all(rv_arena *a, const rv_index *idx,
                          rv_event_class **out, size_t *count)
{
    rv_event_class *arr;
    uint32_t i;
    if (!a || !idx || !out || !count)
        return -1;
    *out = NULL;
    *count = 0;
    if (idx->marker_count == 0)
        return 0;
    arr = (rv_event_class *)rv_arena_calloc(a, idx->marker_count,
                                            sizeof(*arr), 8);
    if (!arr)
        return -1;
    for (i = 0; i < idx->marker_count; i++)
        (void)rv_event_classify_marker(&idx->markers[i], &arr[i]);
    *out = arr;
    *count = idx->marker_count;
    return 0;
}

int rv_event_summarize(const rv_index *idx, rv_event_summary *out)
{
    uint32_t i;
    if (!idx || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    for (i = 0; i < idx->marker_count; i++) {
        rv_event_class c;
        (void)rv_event_classify_marker(&idx->markers[i], &c);
        if (c.kind < 8)
            out->by_kind[c.kind]++;
        if (c.is_actionable)
            out->actionable++;
        if (c.is_ghost_candidate)
            out->ghosts++;
        if (c.severity >= 80)
            out->critical++;
    }
    return 0;
}

int rv_event_filter_actionable(const rv_event_class *in, size_t n,
                               rv_event_class *out, size_t cap, size_t *written)
{
    size_t i, w = 0;
    if (!written)
        return -1;
    for (i = 0; i < n; i++) {
        if (!in[i].is_actionable)
            continue;
        if (out && w < cap)
            out[w] = in[i];
        w++;
    }
    *written = w;
    return 0;
}

int rv_event_detect_ghosts(const rv_parsed *p, const rv_index *idx,
                           uint32_t *out_ids, size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i, j;
    if (!p || !idx || !written)
        return -1;
    /*
     * Ghost heuristic: reflectance events whose distance is approximately
     * twice another strong reflector (multiple reflection).
     */
    for (i = 0; i < idx->marker_count; i++) {
        const rv_marker_index_node *a = &idx->markers[i];
        int ghost = 0;
        if (a->kind != RV_EVT_REFLECT && a->kind != RV_EVT_GHOST)
            continue;
        for (j = 0; j < idx->marker_count; j++) {
            const rv_marker_index_node *b = &idx->markers[j];
            float twice, delta;
            if (i == j || b->window_id != a->window_id)
                continue;
            if (b->kind != RV_EVT_REFLECT)
                continue;
            twice = b->distance_m * 2.0f;
            delta = a->distance_m - twice;
            if (delta < 0)
                delta = -delta;
            if (delta < 5.0f) {
                ghost = 1;
                break;
            }
        }
        if (!ghost)
            continue;
        if (out_ids && n < cap)
            out_ids[n] = a->marker_id;
        n++;
    }
    *written = n;
    (void)p;
    return 0;
}
