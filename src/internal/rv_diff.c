#include "rv_diff.h"

#include <string.h>

static int rv_diff_has_route(const rv_index *idx, uint32_t id)
{
    uint32_t pos;
    return rv_index_find_route(idx, id, &pos) == 0;
}

static const rv_marker_index_node *rv_diff_find_marker(const rv_index *idx,
                                                       uint32_t id)
{
    uint32_t i;
    for (i = 0; i < idx->marker_count; i++) {
        if (idx->markers[i].marker_id == id)
            return &idx->markers[i];
    }
    return NULL;
}

int rv_diff_marker_delta(const rv_index *left, const rv_index *right,
                         uint32_t marker_id, float *loss_delta)
{
    const rv_marker_index_node *a, *b;
    if (!left || !right || !loss_delta)
        return -1;
    a = rv_diff_find_marker(left, marker_id);
    b = rv_diff_find_marker(right, marker_id);
    if (!a || !b || !a->rec || !b->rec)
        return -1;
    *loss_delta = b->rec->loss_db - a->rec->loss_db;
    return 0;
}

int rv_diff_packages(rv_arena *a, const rv_parsed *left, const rv_index *lidx,
                     const rv_parsed *right, const rv_index *ridx,
                     rv_diff_report *out)
{
    size_t cap, n = 0;
    uint32_t i;
    rv_diff_entry *ents;

    if (!a || !left || !right || !lidx || !ridx || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    cap = (size_t)lidx->route_count + ridx->route_count +
          lidx->marker_count + ridx->marker_count + 8;
    ents = (rv_diff_entry *)rv_arena_calloc(a, cap, sizeof(*ents), 8);
    if (!ents)
        return -1;

    for (i = 0; i < lidx->route_count; i++) {
        uint32_t id = lidx->routes[i].route_id;
        if (!rv_diff_has_route(ridx, id)) {
            if (n < cap) {
                ents[n].kind = RV_DIFF_REMOVED;
                ents[n].entity_kind = 1;
                ents[n].id = id;
                n++;
            }
            out->routes_removed++;
        }
    }
    for (i = 0; i < ridx->route_count; i++) {
        uint32_t id = ridx->routes[i].route_id;
        if (!rv_diff_has_route(lidx, id)) {
            if (n < cap) {
                ents[n].kind = RV_DIFF_ADDED;
                ents[n].entity_kind = 1;
                ents[n].id = id;
                n++;
            }
            out->routes_added++;
        }
    }

    for (i = 0; i < lidx->marker_count; i++) {
        uint32_t id = lidx->markers[i].marker_id;
        const rv_marker_index_node *r = rv_diff_find_marker(ridx, id);
        if (!r) {
            if (n < cap) {
                ents[n].kind = RV_DIFF_REMOVED;
                ents[n].entity_kind = 3;
                ents[n].id = id;
                n++;
            }
            out->markers_removed++;
        } else if (lidx->markers[i].rec && r->rec) {
            float dl = r->rec->loss_db - lidx->markers[i].rec->loss_db;
            float dd = r->distance_m - lidx->markers[i].distance_m;
            if (dl < 0)
                dl = -dl;
            if (dd < 0)
                dd = -dd;
            if (dl > 0.05f || dd > 1.0f ||
                r->severity != lidx->markers[i].severity) {
                if (n < cap) {
                    ents[n].kind = RV_DIFF_CHANGED;
                    ents[n].entity_kind = 3;
                    ents[n].id = id;
                    ents[n].delta_loss_db =
                        r->rec->loss_db - lidx->markers[i].rec->loss_db;
                    ents[n].delta_distance_m =
                        r->distance_m - lidx->markers[i].distance_m;
                    n++;
                }
                out->markers_changed++;
            }
        }
    }
    for (i = 0; i < ridx->marker_count; i++) {
        uint32_t id = ridx->markers[i].marker_id;
        if (!rv_diff_find_marker(lidx, id)) {
            if (n < cap) {
                ents[n].kind = RV_DIFF_ADDED;
                ents[n].entity_kind = 3;
                ents[n].id = id;
                n++;
            }
            out->markers_added++;
        }
    }

    out->entries = ents;
    out->count = n;
    (void)left;
    (void)right;
    return 0;
}
