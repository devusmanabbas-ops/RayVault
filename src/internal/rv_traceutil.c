#include "rv_traceutil.h"
#include "rv_waveproc.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

int rv_trace_segmentize(rv_arena *a, const rv_wave_slice *slice,
                        float segment_m, rv_trace_segment **out, size_t *count)
{
    size_t segs, i, samples_per;
    rv_trace_segment *arr;
    if (!a || !slice || !slice->samples || !out || !count || segment_m <= 0.0f ||
        slice->step_m <= 0.0f)
        return -1;
    samples_per = (size_t)(segment_m / slice->step_m);
    if (samples_per < 1)
        samples_per = 1;
    segs = slice->count / samples_per;
    if (segs == 0)
        return -1;
    arr = (rv_trace_segment *)rv_arena_calloc(a, segs, sizeof(*arr), 8);
    if (!arr)
        return -1;
    for (i = 0; i < segs; i++) {
        size_t base = i * samples_per;
        size_t j;
        int16_t mn, mx;
        double sum = 0.0;
        mn = mx = slice->samples[base];
        for (j = 0; j < samples_per; j++) {
            int16_t v = slice->samples[base + j];
            if (v < mn)
                mn = v;
            if (v > mx)
                mx = v;
            sum += v;
        }
        arr[i].start_m = slice->start_m + (float)base * slice->step_m;
        arr[i].end_m = arr[i].start_m + (float)samples_per * slice->step_m;
        arr[i].mean_db_proxy = sum / (double)samples_per;
        arr[i].min_v = mn;
        arr[i].max_v = mx;
        arr[i].window_id = slice->window_id;
    }
    *out = arr;
    *count = segs;
    return 0;
}

int rv_trace_find_dropouts(const rv_wave_slice *slice, int16_t floor,
                           float *out_m, size_t cap, size_t *written)
{
    size_t i, n = 0;
    int in_drop = 0;
    if (!slice || !slice->samples || !written)
        return -1;
    for (i = 0; i < slice->count; i++) {
        if (slice->samples[i] <= floor) {
            if (!in_drop) {
                if (out_m && n < cap)
                    out_m[n] = slice->start_m + (float)i * slice->step_m;
                n++;
                in_drop = 1;
            }
        } else {
            in_drop = 0;
        }
    }
    *written = n;
    return 0;
}

int rv_trace_estimate_length_m(const rv_wave_slice *slice, int16_t eof_threshold,
                               float *out_m)
{
    size_t i;
    if (!slice || !slice->samples || !out_m || slice->count == 0)
        return -1;
    for (i = slice->count; i > 0; i--) {
        if (slice->samples[i - 1] > eof_threshold) {
            *out_m = slice->start_m + (float)(i - 1) * slice->step_m;
            return 0;
        }
    }
    *out_m = slice->start_m;
    return 0;
}

int rv_trace_align_pair(const rv_wave_slice *a, const rv_wave_slice *b,
                        int *out_shift_samples)
{
    /* Coarse cross-correlation over a limited lag window. */
    int best_lag = 0;
    double best = -1e300;
    int lag;
    size_t n;
    if (!a || !b || !a->samples || !b->samples || !out_shift_samples)
        return -1;
    n = a->count < b->count ? a->count : b->count;
    if (n < 8)
        return -1;
    for (lag = -32; lag <= 32; lag++) {
        double corr = 0.0;
        size_t i, used = 0;
        for (i = 0; i < n; i++) {
            int j = (int)i + lag;
            if (j < 0 || (size_t)j >= n)
                continue;
            corr += (double)a->samples[i] * (double)b->samples[j];
            used++;
        }
        if (used && corr > best) {
            best = corr;
            best_lag = lag;
        }
    }
    *out_shift_samples = best_lag;
    return 0;
}

int rv_trace_snr_proxy(const rv_wave_slice *slice, double *out_snr)
{
    rv_wave_stats st;
    double noise;
    if (!slice || !out_snr)
        return -1;
    if (rv_wave_compute_stats(slice, &st) != 0)
        return -1;
    if (rv_wave_estimate_noise_floor(slice, &noise) != 0)
        return -1;
    if (fabs(noise) < 1e-6)
        noise = 1.0;
    *out_snr = fabs(st.mean - noise) / (fabs(noise) + 1.0);
    return 0;
}
