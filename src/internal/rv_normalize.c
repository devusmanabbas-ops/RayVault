#include "rv_normalize.h"

#include <string.h>

static void rv_swap_u32_window(rv_window_rec *a, rv_window_rec *b)
{
    rv_window_rec t = *a;
    *a = *b;
    *b = t;
}

static void rv_sort_windows_by_id(rv_wind_table *t)
{
    uint32_t i, j;
    if (!t || t->count < 2)
        return;
    /* Insertion sort — window counts in field packages are typically modest. */
    for (i = 1; i < t->count; i++) {
        j = i;
        while (j > 0 && t->items[j - 1].window_id > t->items[j].window_id) {
            rv_swap_u32_window(&t->items[j - 1], &t->items[j]);
            j--;
        }
    }
}

int rv_normalize_parsed(rv_parsed *p, rv_normalize_stats *st)
{
    rv_normalize_stats local;
    uint32_t i;
    memset(&local, 0, sizeof(local));
    if (!p)
        return -1;

    rv_sort_windows_by_id(&p->wind);
    local.sorted_windows = p->wind.count;

    for (i = 0; i < p->mark.count; i++) {
        if (p->mark.items[i].severity > 100) {
            p->mark.items[i].severity = 100;
            local.clamped_severity++;
        }
    }

    for (i = 0; i < p->wave.count; i++) {
        rv_wave_block *b = &p->wave.items[i];
        if (b->window_id == 0 && p->wind.count == 1) {
            b->window_id = p->wind.items[0].window_id;
            local.filled_wave_links++;
        }
    }

    for (i = 0; i < p->rout.count; i++) {
        if (p->rout.items[i].route_id == 0)
            local.dropped_zero_routes++;
    }

    if (st)
        *st = local;
    return 0;
}
