#include "rv_waveproc.h"

#include <math.h>
#include <string.h>

int rv_wave_compute_stats(const rv_wave_slice *slice, rv_wave_stats *out)
{
    size_t i;
    double sum = 0.0, sq = 0.0;
    int16_t mn, mx;
    if (!slice || !out || !slice->samples || slice->count == 0)
        return -1;
    mn = mx = slice->samples[0];
    for (i = 0; i < slice->count; i++) {
        int16_t v = slice->samples[i];
        if (v < mn)
            mn = v;
        if (v > mx)
            mx = v;
        sum += v;
        sq += (double)v * (double)v;
    }
    out->min_v = mn;
    out->max_v = mx;
    out->mean = sum / (double)slice->count;
    out->rms = sqrt(sq / (double)slice->count);
    out->sample_count = slice->count;
    out->span_m = (float)((slice->count > 0 ? slice->count - 1 : 0) * slice->step_m);
    return 0;
}

int rv_wave_find_peaks(const rv_wave_slice *slice, int16_t threshold,
                       rv_wave_peak *out, size_t cap, size_t *written)
{
    size_t i, n = 0;
    if (!slice || !slice->samples || !written)
        return -1;
    *written = 0;
    if (slice->count < 3)
        return 0;
    for (i = 1; i + 1 < slice->count; i++) {
        int16_t v = slice->samples[i];
        if (v < threshold)
            continue;
        if (v >= slice->samples[i - 1] && v >= slice->samples[i + 1]) {
            if (out && n < cap) {
                out[n].index = i;
                out[n].distance_m = slice->start_m + (float)i * slice->step_m;
                out[n].value = v;
            }
            n++;
        }
    }
    *written = n;
    return 0;
}

int rv_wave_downsample(rv_arena *a, const rv_wave_slice *in, uint32_t factor,
                       rv_wave_slice *out)
{
    size_t n, i;
    int16_t *buf;
    if (!a || !in || !out || !in->samples || factor == 0)
        return -1;
    n = in->count / factor;
    if (n == 0)
        return -1;
    buf = (int16_t *)rv_arena_alloc(a, n * sizeof(int16_t), 2);
    if (!buf)
        return -1;
    for (i = 0; i < n; i++) {
        size_t base = i * factor;
        size_t j;
        int32_t acc = 0;
        for (j = 0; j < factor; j++)
            acc += in->samples[base + j];
        buf[i] = (int16_t)(acc / (int32_t)factor);
    }
    out->samples = buf;
    out->count = n;
    out->start_m = in->start_m;
    out->step_m = in->step_m * (float)factor;
    out->window_id = in->window_id;
    return 0;
}

int rv_wave_diff(const rv_wave_slice *a, const rv_wave_slice *b,
                 int16_t *out, size_t cap, size_t *written)
{
    size_t n, i;
    if (!a || !b || !a->samples || !b->samples || !written)
        return -1;
    n = a->count < b->count ? a->count : b->count;
    if (out) {
        if (cap < n)
            n = cap;
        for (i = 0; i < n; i++)
            out[i] = (int16_t)(a->samples[i] - b->samples[i]);
    }
    *written = n;
    return 0;
}

int rv_wave_estimate_noise_floor(const rv_wave_slice *slice, double *out)
{
    size_t i, start, end, n = 0;
    double sum = 0.0;
    if (!slice || !out || !slice->samples || slice->count < 16)
        return -1;
    /* Use last 10% of trace as noise estimate (OTDR convention). */
    start = (slice->count * 9) / 10;
    end = slice->count;
    for (i = start; i < end; i++) {
        sum += slice->samples[i];
        n++;
    }
    *out = n ? (sum / (double)n) : 0.0;
    return 0;
}

int rv_wave_apply_offset(rv_wave_slice *slice, int16_t offset)
{
    /* Only valid when samples are arena-owned mutable buffers. */
    size_t i;
    int16_t *mut;
    if (!slice || !slice->samples)
        return -1;
    mut = (int16_t *)(void *)slice->samples;
    for (i = 0; i < slice->count; i++)
        mut[i] = (int16_t)(mut[i] + offset);
    return 0;
}
