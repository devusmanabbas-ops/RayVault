#ifndef RV_TRACEUTIL_H
#define RV_TRACEUTIL_H

#include "rayvault/rayvault_types.h"
#include "rv_arena.h"
#include "rv_parser.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_trace_segment {
    float    start_m;
    float    end_m;
    double   mean_db_proxy;
    int16_t  min_v;
    int16_t  max_v;
    uint32_t window_id;
} rv_trace_segment;

int rv_trace_segmentize(rv_arena *a, const rv_wave_slice *slice,
                        float segment_m, rv_trace_segment **out, size_t *count);
int rv_trace_find_dropouts(const rv_wave_slice *slice, int16_t floor,
                           float *out_m, size_t cap, size_t *written);
int rv_trace_estimate_length_m(const rv_wave_slice *slice, int16_t eof_threshold,
                               float *out_m);
int rv_trace_align_pair(const rv_wave_slice *a, const rv_wave_slice *b,
                        int *out_shift_samples);
int rv_trace_snr_proxy(const rv_wave_slice *slice, double *out_snr);

#endif /* RV_TRACEUTIL_H */
