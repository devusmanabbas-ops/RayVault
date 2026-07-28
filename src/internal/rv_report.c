#include "rv_report.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void rv_report_options_default(rv_report_options *o)
{
    if (!o)
        return;
    memset(o, 0, sizeof(*o));
    o->include_routes = 1;
    o->include_events = 1;
    o->include_histogram = 1;
    o->include_wave_summary = 1;
    o->json_style = 0;
}

static int rv_report_printf(rv_buf *out, const char *fmt, ...)
{
    char tmp[512];
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

int rv_report_render_route(rv_buf *out, const rv_route_health *rh)
{
    if (!out || !rh)
        return -1;
    return rv_report_printf(
        out,
        "route %u site=%s span=%.3fkm windows=%u markers=%u critical=%u "
        "max_loss=%.3fdB mean_loss=%.3fdB\n",
        rh->route_id, rh->site_label ? rh->site_label : "-", rh->span_km,
        rh->window_count, rh->marker_count, rh->critical_markers,
        rh->max_loss_db, rh->mean_loss_db);
}

int rv_report_render_marker_line(rv_buf *out, const rv_event_class *ec)
{
    if (!out || !ec)
        return -1;
    return rv_report_printf(
        out, "  marker %u kind=%s sev=%u dist=%.1fm loss=%.3f refl=%.1f %s%s\n",
        ec->marker_id, rv_event_kind_name(ec->kind), ec->severity,
        ec->distance_m, ec->loss_db, ec->reflectance_db,
        ec->is_actionable ? "ACTION " : "",
        ec->is_ghost_candidate ? "GHOST" : "");
}

int rv_report_render(rv_buf *out, const rv_parsed *p, const rv_index *idx,
                     const rv_plant_report *plant, const rv_event_summary *ev,
                     const rv_report_options *opt)
{
    rv_report_options def;
    uint32_t i;
    if (!out || !p || !idx)
        return -1;
    if (!opt) {
        rv_report_options_default(&def);
        opt = &def;
    }

    if (opt->json_style) {
        if (rv_report_printf(out, "{\"routes\":%u,\"windows\":%u,\"markers\":%u",
                             p->rout.count, p->wind.count, p->mark.count) != 0)
            return -1;
        if (ev && rv_report_printf(out, ",\"actionable\":%u,\"critical\":%u",
                                   ev->actionable, ev->critical) != 0)
            return -1;
        if (rv_report_printf(out, "}\n") != 0)
            return -1;
        return 0;
    }

    if (rv_report_printf(out, "RayVault plant report\n") != 0)
        return -1;
    if (rv_report_printf(out, "routes=%u instruments=%u windows=%u waves=%u "
                             "markers=%u names=%u\n",
                         p->rout.count, p->inst.count, p->wind.count,
                         p->wave.count, p->mark.count, p->snam.count) != 0)
        return -1;

    if (opt->include_routes && plant) {
        if (rv_report_printf(out, "plant_km=%.3f\n", plant->plant_km) != 0)
            return -1;
        for (i = 0; i < plant->route_count; i++) {
            if (rv_report_render_route(out, &plant->routes[i]) != 0)
                return -1;
        }
    }

    if (opt->include_events && ev) {
        if (rv_report_printf(out,
                             "events actionable=%u ghosts=%u critical=%u\n",
                             ev->actionable, ev->ghosts, ev->critical) != 0)
            return -1;
        for (i = 0; i < 8; i++) {
            if (ev->by_kind[i] == 0)
                continue;
            if (rv_report_printf(out, "  %-12s %u\n", rv_event_kind_name((uint16_t)i),
                                 ev->by_kind[i]) != 0)
                return -1;
        }
    }

    if (opt->include_histogram && plant) {
        if (rv_report_printf(out, "loss histogram (0.5 dB buckets):\n") != 0)
            return -1;
        for (i = 0; i < 16; i++) {
            if (plant->hist.buckets[i] == 0)
                continue;
            if (rv_report_printf(out, "  [%.1f,%.1f) %u\n", plant->hist.edges[i],
                                 plant->hist.edges[i + 1],
                                 plant->hist.buckets[i]) != 0)
                return -1;
        }
    }

    if (opt->include_wave_summary) {
        uint64_t samples = 0;
        for (i = 0; i < p->wave.count; i++)
            samples += p->wave.items[i].sample_count;
        if (rv_report_printf(out, "total_samples=%llu wave_blocks=%u\n",
                             (unsigned long long)samples, p->wave.count) != 0)
            return -1;
    }

    return 0;
}
