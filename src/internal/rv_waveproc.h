#ifndef RV_WAVEPROC_H
#define RV_WAVEPROC_H

#include "rayvault/rayvault_types.h"
#include "rv_arena.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_wave_stats {
    int16_t  min_v;
    int16_t  max_v;
    double   mean;
    double   rms;
    size_t   sample_count;
    float    span_m;
} rv_wave_stats;

typedef struct rv_wave_peak {
    size_t index;
    float  distance_m;
    int16_t value;
} rv_wave_peak;

int rv_wave_compute_stats(const rv_wave_slice *slice, rv_wave_stats *out);
int rv_wave_find_peaks(const rv_wave_slice *slice, int16_t threshold,
                       rv_wave_peak *out, size_t cap, size_t *written);
int rv_wave_downsample(rv_arena *a, const rv_wave_slice *in, uint32_t factor,
                       rv_wave_slice *out);
int rv_wave_diff(const rv_wave_slice *a, const rv_wave_slice *b,
                 int16_t *out, size_t cap, size_t *written);
int rv_wave_estimate_noise_floor(const rv_wave_slice *slice, double *out);
int rv_wave_apply_offset(rv_wave_slice *slice, int16_t offset);

#endif /* RV_WAVEPROC_H */
