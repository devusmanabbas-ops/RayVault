#include "rv_stats.h"

#include <string.h>

int rv_stats_compute(const rv_parsed *p, const rv_index *idx,
                     rv_stats_snapshot *out)
{
    uint32_t i;
    double sum_km = 0.0;
    double max_loss = 0.0;
    uint64_t samples = 0;

    if (!p || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    out->routes = p->rout.count;
    out->instruments = p->inst.count;
    out->calibrations = p->clbr.count;
    out->windows = p->wind.count;
    out->wave_blocks = p->wave.count;
    out->markers = p->mark.count;
    out->linkages = p->link.count;
    out->name_entries = p->snam.count;

    for (i = 0; i < p->rout.count; i++)
        sum_km += p->rout.items[i].length_km;
    out->mean_span_km = p->rout.count ? (sum_km / p->rout.count) : 0.0;

    for (i = 0; i < p->mark.count; i++) {
        if (p->mark.items[i].loss_db > max_loss)
            max_loss = p->mark.items[i].loss_db;
    }
    out->max_loss_db = max_loss;

    for (i = 0; i < p->wave.count; i++)
        samples += p->wave.items[i].sample_count;
    out->total_samples = samples;
    (void)idx;
    return 0;
}

int rv_derived_summary_build(rv_derived_summary *ds, rv_arena *a,
                             const rv_parsed *p, const rv_index *idx)
{
    uint32_t i;
    if (!ds || !a || !p || !idx)
        return -1;
    memset(ds, 0, sizeof(*ds));
    if (rv_stats_compute(p, idx, &ds->snap) != 0)
        return -1;
    ds->label_count = idx->route_count;
    if (ds->label_count) {
        ds->route_labels = (const char **)rv_arena_calloc(
            a, ds->label_count, sizeof(char *), sizeof(void *));
        if (!ds->route_labels)
            return -1;
        for (i = 0; i < ds->label_count; i++) {
            /* Borrow site_name pointers from the index nodes. */
            ds->route_labels[i] = idx->routes[i].site_name;
        }
    }
    ds->index_generation = idx->generation;
    ds->valid = 1;
    return 0;
}

void rv_derived_summary_invalidate(rv_derived_summary *ds)
{
    if (!ds)
        return;
    ds->valid = 0;
    ds->route_labels = NULL;
    ds->label_count = 0;
}
