#include "rv_acqmeta.h"

#include <stdint.h>
#include <string.h>

static const rv_window_rec *rv_find_window_rec(const rv_parsed *p, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < p->wind.count; i++) {
        if (p->wind.items[i].window_id == id)
            return &p->wind.items[i];
    }
    return NULL;
}

static const rv_inst_rec *rv_find_inst_rec(const rv_parsed *p, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < p->inst.count; i++) {
        if (p->inst.items[i].inst_id == id)
            return &p->inst.items[i];
    }
    return NULL;
}

static const rv_calib_rec *rv_find_calib_rec(const rv_parsed *p, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < p->clbr.count; i++) {
        if (p->clbr.items[i].calib_id == id)
            return &p->clbr.items[i];
    }
    return NULL;
}

int rv_acq_context_for_window(const rv_parsed *p, uint32_t window_id,
                              rv_acq_context *out)
{
    const rv_window_rec *w;
    const rv_inst_rec *inst;
    const rv_calib_rec *cal;
    if (!p || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    w = rv_find_window_rec(p, window_id);
    if (!w)
        return -1;
    out->window_id = w->window_id;
    out->route_id = w->route_id;
    out->inst_id = w->inst_id;
    out->calib_id = w->calib_id;
    out->pulse_ns = w->pulse_ns;
    out->range_km = w->range_km;
    out->resolution_m = w->resolution_m;
    out->acquired_unix = w->acquired_unix;
    inst = rv_find_inst_rec(p, w->inst_id);
    if (inst)
        out->wavelength_nm = inst->wavelength_nm;
    cal = rv_find_calib_rec(p, w->calib_id);
    if (cal) {
        out->refractive_index = cal->refractive_index;
        out->calibrated_unix = cal->calibrated_unix;
        if (w->acquired_unix > cal->calibrated_unix + 365ull * 24ull * 3600ull)
            out->calib_stale = 1;
    }
    return 0;
}

int rv_acq_list_stale_calibrations(const rv_parsed *p, uint32_t *out_inst,
                                   size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i;
    if (!p || !written)
        return -1;
    for (i = 0; i < p->wind.count; i++) {
        rv_acq_context ctx;
        size_t j;
        int seen = 0;
        if (rv_acq_context_for_window(p, p->wind.items[i].window_id, &ctx) != 0)
            continue;
        if (!ctx.calib_stale)
            continue;
        for (j = 0; j < n; j++) {
            if (out_inst && out_inst[j] == ctx.inst_id) {
                seen = 1;
                break;
            }
        }
        if (seen)
            continue;
        if (out_inst && n < cap)
            out_inst[n] = ctx.inst_id;
        n++;
    }
    *written = n;
    return 0;
}

int rv_acq_group_windows_by_day(const rv_parsed *p, uint64_t *day_keys,
                                uint32_t *counts, size_t cap, size_t *written)
{
    size_t n = 0;
    uint32_t i, j;
    if (!p || !written)
        return -1;
    for (i = 0; i < p->wind.count; i++) {
        uint64_t day = p->wind.items[i].acquired_unix / 86400ull;
        int found = 0;
        for (j = 0; j < n; j++) {
            if (day_keys && day_keys[j] == day) {
                if (counts)
                    counts[j]++;
                found = 1;
                break;
            }
        }
        if (found)
            continue;
        if (day_keys && n < cap)
            day_keys[n] = day;
        if (counts && n < cap)
            counts[n] = 1;
        n++;
        if (cap && n >= cap)
            break;
    }
    *written = n;
    return 0;
}

uint64_t rv_acq_span_duration_sec(const rv_parsed *p, uint32_t route_id)
{
    uint64_t mn = UINT64_MAX, mx = 0;
    uint32_t i;
    int any = 0;
    for (i = 0; i < p->wind.count; i++) {
        if (route_id && p->wind.items[i].route_id != route_id)
            continue;
        any = 1;
        if (p->wind.items[i].acquired_unix < mn)
            mn = p->wind.items[i].acquired_unix;
        if (p->wind.items[i].acquired_unix > mx)
            mx = p->wind.items[i].acquired_unix;
    }
    if (!any || mx < mn)
        return 0;
    return mx - mn;
}
