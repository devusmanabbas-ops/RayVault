#ifndef RV_FILTER_H
#define RV_FILTER_H

#include "rv_index.h"
#include "rv_parser.h"
#include "rv_arena.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_span_filter {
    uint32_t route_id;
    uint32_t inst_id;
    uint64_t acquired_after;
    uint64_t acquired_before;
    float    min_range_km;
    float    max_range_km;
    uint16_t wavelength_nm;
} rv_span_filter;

typedef struct rv_filtered_windows {
    uint32_t *ids;
    size_t    count;
} rv_filtered_windows;

void rv_span_filter_init(rv_span_filter *f);
int  rv_span_filter_windows(rv_arena *a, const rv_parsed *p, const rv_index *idx,
                            const rv_span_filter *f, rv_filtered_windows *out);
int  rv_span_filter_match_window(const rv_parsed *p, const rv_window_rec *w,
                                 const rv_span_filter *f);
int  rv_span_filter_apply_to_markers(const rv_index *idx,
                                     const rv_filtered_windows *wins,
                                     rv_marker_info *out, size_t cap,
                                     size_t *written);

#endif /* RV_FILTER_H */
