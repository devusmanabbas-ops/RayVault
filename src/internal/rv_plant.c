#include "rv_plant.h"
#include "rv_dict.h"

#include <string.h>

int rv_plant_build_graph(rv_arena *a, const rv_parsed *p, const rv_index *idx,
                         rv_plant_graph *out)
{
    uint32_t i, ecount = 0;
    if (!a || !p || !idx || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    out->node_count = idx->route_count;
    if (out->node_count) {
        out->nodes = (rv_plant_node *)rv_arena_calloc(a, out->node_count,
                                                      sizeof(*out->nodes), 8);
        if (!out->nodes)
            return -1;
    }
    for (i = 0; i < idx->route_count; i++) {
        rv_plant_node *n = &out->nodes[i];
        const rv_route_rec *r = idx->routes[i].rec;
        const rv_name_entry *site, *far;
        uint32_t w;
        n->route_id = idx->routes[i].route_id;
        n->site_label = idx->routes[i].site_name;
        if (r) {
            n->site_name_id = r->site_name_id;
            n->far_name_id = r->far_name_id;
            n->length_km = r->length_km;
            out->total_km += r->length_km;
            site = rv_dict_by_id(&p->snam, r->site_name_id);
            far = rv_dict_by_id(&p->snam, r->far_name_id);
            if (site)
                n->site_label = site->str;
            if (far)
                n->far_label = far->str;
        }
        for (w = 0; w < idx->window_count; w++) {
            if (idx->windows[w].route_id == n->route_id)
                n->degree++;
        }
    }

    /* Edges from LINK records that connect routes (kind 1 -> kind 1). */
    for (i = 0; i < p->link.count; i++) {
        if (p->link.items[i].from_kind == 1 && p->link.items[i].to_kind == 1)
            ecount++;
    }
    out->edge_count = ecount;
    if (ecount) {
        uint32_t e = 0;
        out->edges = (rv_plant_edge *)rv_arena_calloc(a, ecount,
                                                      sizeof(*out->edges), 8);
        if (!out->edges)
            return -1;
        for (i = 0; i < p->link.count; i++) {
            const rv_link_rec *l = &p->link.items[i];
            if (l->from_kind != 1 || l->to_kind != 1)
                continue;
            out->edges[e].from_route = l->from_id;
            out->edges[e].to_route = l->to_id;
            out->edges[e].via_link_id = l->link_id;
            out->edges[e].shared_hint_km = 0.0f;
            e++;
        }
    }
    return 0;
}

int rv_plant_longest_path_km(const rv_plant_graph *g, double *out_km)
{
    uint32_t i;
    double best = 0.0;
    if (!g || !out_km)
        return -1;
    for (i = 0; i < g->node_count; i++) {
        if (g->nodes[i].length_km > best)
            best = g->nodes[i].length_km;
    }
    /* Approximate: sum of two longest routes if an edge exists. */
    if (g->edge_count && g->node_count >= 2) {
        double a = 0, b = 0;
        for (i = 0; i < g->node_count; i++) {
            double L = g->nodes[i].length_km;
            if (L >= a) {
                b = a;
                a = L;
            } else if (L > b) {
                b = L;
            }
        }
        if (a + b > best)
            best = a + b;
    }
    *out_km = best;
    return 0;
}

int rv_plant_find_hubs(const rv_plant_graph *g, uint32_t *out_route_ids,
                       size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i;
    uint32_t max_deg = 0;
    if (!g || !written)
        return -1;
    for (i = 0; i < g->node_count; i++) {
        if (g->nodes[i].degree > max_deg)
            max_deg = g->nodes[i].degree;
    }
    for (i = 0; i < g->node_count; i++) {
        if (max_deg == 0 || g->nodes[i].degree < max_deg)
            continue;
        if (out_route_ids && n < cap)
            out_route_ids[n] = g->nodes[i].route_id;
        n++;
    }
    *written = n;
    return 0;
}

int rv_plant_orphan_routes(const rv_parsed *p, const rv_index *idx,
                           uint32_t *out, size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i, j;
    if (!p || !idx || !written)
        return -1;
    for (i = 0; i < p->rout.count; i++) {
        int has_win = 0;
        for (j = 0; j < p->wind.count; j++) {
            if (p->wind.items[j].route_id == p->rout.items[i].route_id) {
                has_win = 1;
                break;
            }
        }
        if (has_win)
            continue;
        if (out && n < cap)
            out[n] = p->rout.items[i].route_id;
        n++;
    }
    *written = n;
    return 0;
}

int rv_plant_fiber_capacity(const rv_parsed *p, uint32_t *out_total_fibers)
{
    uint32_t i, sum = 0;
    if (!p || !out_total_fibers)
        return -1;
    for (i = 0; i < p->rout.count; i++)
        sum += p->rout.items[i].fiber_count;
    *out_total_fibers = sum;
    return 0;
}
