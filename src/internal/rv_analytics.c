#include "rv_analytics.h"
#include "rv_dict.h"

#include <stdlib.h>
#include <string.h>

int rv_analytics_loss_histogram(const rv_parsed *p, rv_loss_histogram *h)
{
    uint32_t i, b;
    if (!p || !h)
        return -1;
    memset(h, 0, sizeof(*h));
    for (i = 0; i < 17; i++)
        h->edges[i] = (float)i * 0.5f; /* 0 .. 8 dB */
    for (i = 0; i < p->mark.count; i++) {
        float loss = p->mark.items[i].loss_db;
        if (loss < 0.0f)
            loss = 0.0f;
        b = (uint32_t)(loss / 0.5f);
        if (b > 15)
            b = 15;
        h->buckets[b]++;
        h->total++;
    }
    return 0;
}

int rv_analytics_route_health(rv_arena *a, const rv_parsed *p,
                              const rv_index *idx, rv_plant_report *out)
{
    uint32_t i, j;
    if (!a || !p || !idx || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    out->route_count = idx->route_count;
    if (out->route_count) {
        out->routes = (rv_route_health *)rv_arena_calloc(
            a, out->route_count, sizeof(rv_route_health), 8);
        if (!out->routes)
            return -1;
    }
    for (i = 0; i < idx->route_count; i++) {
        rv_route_health *rh = &out->routes[i];
        double sum_loss = 0.0;
        uint32_t nloss = 0;
        rh->route_id = idx->routes[i].route_id;
        rh->span_km = idx->routes[i].rec ? idx->routes[i].rec->length_km : 0;
        rh->site_label = idx->routes[i].site_name;
        out->plant_km += rh->span_km;

        for (j = 0; j < idx->window_count; j++) {
            if (idx->windows[j].route_id == rh->route_id)
                rh->window_count++;
        }
        for (j = 0; j < idx->marker_count; j++) {
            uint32_t wpos;
            if (rv_index_find_window(idx, idx->markers[j].window_id, &wpos) != 0)
                continue;
            if (idx->windows[wpos].route_id != rh->route_id)
                continue;
            rh->marker_count++;
            if (idx->markers[j].severity >= 80)
                rh->critical_markers++;
            if (idx->markers[j].rec) {
                float loss = idx->markers[j].rec->loss_db;
                sum_loss += loss;
                nloss++;
                if (loss > rh->max_loss_db)
                    rh->max_loss_db = loss;
            }
            switch (idx->markers[j].kind) {
            case 1: out->splice_events++; break;
            case 2: out->reflect_events++; break;
            case 3: out->open_events++; break;
            default: break;
            }
        }
        rh->mean_loss_db = nloss ? (float)(sum_loss / nloss) : 0.0f;
    }
    (void)rv_analytics_loss_histogram(p, &out->hist);
    return 0;
}

int rv_analytics_top_loss_markers(const rv_index *idx, rv_marker_info *out,
                                  size_t cap, size_t *written)
{
    /* Simple selection of highest-loss markers; O(n*cap). */
    size_t n = 0;
    uint32_t i, k;
    if (!idx || !written)
        return -1;
    *written = 0;
    if (!out || cap == 0)
        return 0;
    for (i = 0; i < idx->marker_count; i++) {
        const rv_marker_index_node *m = &idx->markers[i];
        float loss = m->rec ? m->rec->loss_db : 0.0f;
        size_t insert = n;
        if (n < cap) {
            insert = n++;
        } else {
            size_t worst = 0;
            float worst_loss = out[0].distance_m; /* placeholder reuse */
            worst_loss = 0;
            for (k = 0; k < n; k++) {
                /* Find lowest loss currently stored to maybe replace. */
                float cur = 0;
                uint32_t j;
                for (j = 0; j < idx->marker_count; j++) {
                    if (idx->markers[j].marker_id == out[k].marker_id) {
                        cur = idx->markers[j].rec ? idx->markers[j].rec->loss_db
                                                  : 0;
                        break;
                    }
                }
                if (k == 0 || cur < worst_loss) {
                    worst_loss = cur;
                    worst = k;
                }
            }
            if (loss <= worst_loss)
                continue;
            insert = worst;
        }
        out[insert].marker_id = m->marker_id;
        out[insert].window_id = m->window_id;
        out[insert].distance_m = m->distance_m;
        out[insert].kind = m->kind;
        out[insert].severity = m->severity;
        out[insert].label_name_id = m->rec ? m->rec->label_name_id : 0;
        /* Touch label string (arena-backed) for downstream report printers. */
        if (m->label)
            (void)m->label[0];
    }
    *written = n;
    return 0;
}

int rv_analytics_wavelength_coverage(const rv_parsed *p, uint16_t *waves,
                                     size_t cap, size_t *written)
{
    uint32_t i;
    size_t n = 0;
    if (!p || !written)
        return -1;
    *written = 0;
    for (i = 0; i < p->inst.count; i++) {
        uint16_t w = p->inst.items[i].wavelength_nm;
        size_t j;
        int seen = 0;
        for (j = 0; j < n; j++) {
            if (waves && waves[j] == w) {
                seen = 1;
                break;
            }
        }
        if (seen)
            continue;
        if (waves && n < cap)
            waves[n] = w;
        n++;
    }
    *written = n;
    return 0;
}

double rv_analytics_estimate_attenuation(const rv_parsed *p, uint32_t route_id)
{
    uint32_t i;
    double loss_sum = 0.0;
    double km = 0.0;
    for (i = 0; i < p->rout.count; i++) {
        if (p->rout.items[i].route_id == route_id) {
            km = p->rout.items[i].length_km;
            break;
        }
    }
    if (km <= 0.0)
        return 0.0;
    for (i = 0; i < p->mark.count; i++) {
        uint32_t w, found = 0;
        for (w = 0; w < p->wind.count; w++) {
            if (p->wind.items[w].window_id == p->mark.items[i].window_id &&
                p->wind.items[w].route_id == route_id) {
                found = 1;
                break;
            }
        }
        if (found)
            loss_sum += p->mark.items[i].loss_db;
    }
    return loss_sum / km;
}
