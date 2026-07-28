#include "rv_textexport.h"
#include "rv_dict.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int rv_te_printf(rv_buf *out, const char *fmt, ...)
{
    char tmp[768];
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0)
        return -1;
    if ((size_t)n >= sizeof(tmp))
        n = (int)sizeof(tmp) - 1;
    return rv_buf_append(out, tmp, (size_t)n);
}

int rv_textexport_routes_csv(rv_buf *out, const rv_parsed *p,
                             const rv_index *idx)
{
    uint32_t i;
    if (!out || !p || !idx)
        return -1;
    if (rv_te_printf(out, "route_id,site,far,length_km,fibers\n") != 0)
        return -1;
    for (i = 0; i < idx->route_count; i++) {
        const rv_route_index_node *n = &idx->routes[i];
        const rv_route_rec *r = n->rec;
        const char *far = "-";
        if (r) {
            const rv_name_entry *e = NULL;
            /* far label resolved lazily through parsed SNAM */
            uint32_t j;
            for (j = 0; j < p->snam.count; j++) {
                if (p->snam.entries[j].id == r->far_name_id) {
                    far = p->snam.entries[j].str;
                    break;
                }
            }
            (void)e;
            if (rv_te_printf(out, "%u,%s,%s,%.4f,%u\n", n->route_id,
                             n->site_name ? n->site_name : "-", far,
                             r->length_km, r->fiber_count) != 0)
                return -1;
        }
    }
    return 0;
}

int rv_textexport_markers_csv(rv_buf *out, const rv_index *idx)
{
    uint32_t i;
    if (!out || !idx)
        return -1;
    if (rv_te_printf(out, "marker_id,window_id,kind,severity,distance_m,loss_db,label\n") != 0)
        return -1;
    for (i = 0; i < idx->marker_count; i++) {
        const rv_marker_index_node *m = &idx->markers[i];
        float loss = m->rec ? m->rec->loss_db : 0.0f;
        if (rv_te_printf(out, "%u,%u,%u,%u,%.3f,%.4f,%s\n", m->marker_id,
                         m->window_id, m->kind, m->severity, m->distance_m, loss,
                         m->label ? m->label : "") != 0)
            return -1;
    }
    return 0;
}

int rv_textexport_windows_csv(rv_buf *out, const rv_parsed *p)
{
    uint32_t i;
    if (!out || !p)
        return -1;
    if (rv_te_printf(out, "window_id,route_id,inst_id,calib_id,wave_block_id,"
                          "pulse_ns,range_km,resolution_m,acquired_unix\n") != 0)
        return -1;
    for (i = 0; i < p->wind.count; i++) {
        const rv_window_rec *w = &p->wind.items[i];
        if (rv_te_printf(out, "%u,%u,%u,%u,%u,%.3f,%.4f,%.4f,%llu\n",
                         w->window_id, w->route_id, w->inst_id, w->calib_id,
                         w->wave_block_id, w->pulse_ns, w->range_km,
                         w->resolution_m,
                         (unsigned long long)w->acquired_unix) != 0)
            return -1;
    }
    return 0;
}

int rv_textexport_summary_txt(rv_buf *out, const rv_parsed *p,
                              const rv_plant_report *plant,
                              const rv_event_summary *ev)
{
    if (!out || !p)
        return -1;
    if (rv_te_printf(out, "RayVault text summary\n") != 0)
        return -1;
    if (rv_te_printf(out, "routes=%u windows=%u markers=%u waves=%u\n",
                     p->rout.count, p->wind.count, p->mark.count,
                     p->wave.count) != 0)
        return -1;
    if (plant && rv_te_printf(out, "plant_km=%.3f splice=%u reflect=%u open=%u\n",
                              plant->plant_km, plant->splice_events,
                              plant->reflect_events, plant->open_events) != 0)
        return -1;
    if (ev && rv_te_printf(out, "actionable=%u ghosts=%u critical=%u\n",
                           ev->actionable, ev->ghosts, ev->critical) != 0)
        return -1;
    return 0;
}
