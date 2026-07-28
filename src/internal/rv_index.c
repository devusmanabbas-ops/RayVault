#include "rv_index.h"
#include "rv_dict.h"
#include "rv_log.h"

#include <stdlib.h>
#include <string.h>

void rv_index_clear(rv_index *idx)
{
    if (!idx)
        return;
    memset(idx, 0, sizeof(*idx));
}

static int rv_idmap_put(uint32_t **map, uint32_t *cap, rv_arena *a,
                        uint32_t id, uint32_t pos_plus)
{
    uint32_t need;
    if (id == 0)
        return 0;
    need = id + 1;
    if (need > *cap) {
        uint32_t ncap = *cap ? *cap : 32;
        uint32_t *nmap;
        while (ncap < need) {
            if (ncap > 0x7fffffffu / 2)
                return -1;
            ncap *= 2;
        }
        nmap = (uint32_t *)rv_arena_calloc(a, ncap, sizeof(uint32_t), 4);
        if (!nmap)
            return -1;
        if (*map && *cap)
            memcpy(nmap, *map, (*cap) * sizeof(uint32_t));
        *map = nmap;
        *cap = ncap;
    }
    (*map)[id] = pos_plus;
    return 0;
}

int rv_index_build(rv_index *idx, rv_arena *a, const rv_parsed *parsed)
{
    uint32_t i;
    if (!idx || !a || !parsed)
        return -1;
    rv_index_clear(idx);

    idx->route_count = parsed->rout.count;
    if (idx->route_count) {
        idx->routes = (rv_route_index_node *)rv_arena_calloc(
            a, idx->route_count, sizeof(*idx->routes), 8);
        if (!idx->routes)
            return -1;
        for (i = 0; i < idx->route_count; i++) {
            const rv_route_rec *r = &parsed->rout.items[i];
            const rv_name_entry *ne;
            idx->routes[i].route_id = r->route_id;
            idx->routes[i].table_index = i;
            idx->routes[i].rec = r;
            ne = rv_dict_by_id(&parsed->snam, r->site_name_id);
            /* Retain interned pointer — valid for arena epoch. */
            idx->routes[i].site_name = ne ? ne->str : NULL;
            if (rv_idmap_put(&idx->route_by_id, &idx->route_map_cap, a,
                             r->route_id, i + 1) != 0)
                return -1;
        }
    }

    idx->window_count = parsed->wind.count;
    if (idx->window_count) {
        idx->windows = (rv_window_index_node *)rv_arena_calloc(
            a, idx->window_count, sizeof(*idx->windows), 8);
        if (!idx->windows)
            return -1;
        for (i = 0; i < idx->window_count; i++) {
            const rv_window_rec *w = &parsed->wind.items[i];
            idx->windows[i].window_id = w->window_id;
            idx->windows[i].route_id = w->route_id;
            idx->windows[i].inst_id = w->inst_id;
            idx->windows[i].table_index = i;
            idx->windows[i].rec = w;
            if (rv_idmap_put(&idx->window_by_id, &idx->window_map_cap, a,
                             w->window_id, i + 1) != 0)
                return -1;
        }
    }

    idx->wave_count = parsed->wave.count;
    if (idx->wave_count) {
        idx->waves = (rv_wave_index_node *)rv_arena_calloc(
            a, idx->wave_count, sizeof(*idx->waves), 8);
        if (!idx->waves)
            return -1;
        for (i = 0; i < idx->wave_count; i++) {
            const rv_wave_block *b = &parsed->wave.items[i];
            idx->waves[i].block_id = b->block_id;
            idx->waves[i].window_id = b->window_id;
            idx->waves[i].table_index = i;
            idx->waves[i].block = b;
            if (rv_idmap_put(&idx->wave_by_id, &idx->wave_map_cap, a,
                             b->block_id, i + 1) != 0)
                return -1;
        }
    }

    idx->marker_count = parsed->mark.count;
    if (idx->marker_count) {
        idx->markers = (rv_marker_index_node *)rv_arena_calloc(
            a, idx->marker_count, sizeof(*idx->markers), 8);
        if (!idx->markers)
            return -1;
        for (i = 0; i < idx->marker_count; i++) {
            const rv_marker_rec *m = &parsed->mark.items[i];
            const rv_name_entry *ne;
            idx->markers[i].marker_id = m->marker_id;
            idx->markers[i].window_id = m->window_id;
            idx->markers[i].kind = m->kind;
            idx->markers[i].severity = m->severity;
            idx->markers[i].distance_m = m->distance_m;
            idx->markers[i].rec = m;
            ne = rv_dict_by_id(&parsed->snam, m->label_name_id);
            idx->markers[i].label = ne ? ne->str : NULL;
        }
    }

    idx->generation++;
    idx->ready = 1;
    return 0;
}

int rv_index_find_window(const rv_index *idx, uint32_t window_id,
                         uint32_t *out_pos)
{
    if (!idx || !idx->ready || !out_pos)
        return -1;
    if (!idx->window_by_id || window_id >= idx->window_map_cap ||
        idx->window_by_id[window_id] == 0)
        return -1;
    *out_pos = idx->window_by_id[window_id] - 1;
    return 0;
}

int rv_index_find_wave(const rv_index *idx, uint32_t block_id,
                       uint32_t *out_pos)
{
    if (!idx || !idx->ready || !out_pos)
        return -1;
    if (!idx->wave_by_id || block_id >= idx->wave_map_cap ||
        idx->wave_by_id[block_id] == 0)
        return -1;
    *out_pos = idx->wave_by_id[block_id] - 1;
    return 0;
}

int rv_index_find_route(const rv_index *idx, uint32_t route_id,
                        uint32_t *out_pos)
{
    if (!idx || !idx->ready || !out_pos)
        return -1;
    if (!idx->route_by_id || route_id >= idx->route_map_cap ||
        idx->route_by_id[route_id] == 0)
        return -1;
    *out_pos = idx->route_by_id[route_id] - 1;
    return 0;
}

int rv_index_query_markers(const rv_index *idx, const rv_query_filter *flt,
                           rv_marker_info *out, size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i;
    if (!idx || !idx->ready || !written)
        return -1;
    *written = 0;
    for (i = 0; i < idx->marker_count; i++) {
        const rv_marker_index_node *m = &idx->markers[i];
        uint32_t route_id = 0;
        uint32_t wpos;

        if (flt) {
            if (flt->marker_kind && m->kind != flt->marker_kind)
                continue;
            if (m->severity < flt->min_severity)
                continue;
            if (flt->min_distance_m != 0.0f || flt->max_distance_m != 0.0f) {
                if (m->distance_m < flt->min_distance_m)
                    continue;
                if (flt->max_distance_m > 0.0f &&
                    m->distance_m > flt->max_distance_m)
                    continue;
            }
            if (flt->route_id) {
                if (rv_index_find_window(idx, m->window_id, &wpos) != 0)
                    continue;
                route_id = idx->windows[wpos].route_id;
                if (route_id != flt->route_id)
                    continue;
            }
            if (flt->instrument_id) {
                if (rv_index_find_window(idx, m->window_id, &wpos) != 0)
                    continue;
                if (idx->windows[wpos].inst_id != flt->instrument_id)
                    continue;
            }
        }
        if (out && n < cap) {
            out[n].marker_id = m->marker_id;
            out[n].window_id = m->window_id;
            out[n].distance_m = m->distance_m;
            out[n].kind = m->kind;
            out[n].severity = m->severity;
            out[n].label_name_id = m->rec ? m->rec->label_name_id : 0;
            /* Touch label to ensure string is reachable for consumers. */
            if (m->label && m->label[0] == '\0' && m->label[0] != '\0') {
                /* unreachable; keeps optimizer from dropping the borrow */
            }
            (void)m->label;
        }
        n++;
    }
    *written = n;
    return 0;
}
