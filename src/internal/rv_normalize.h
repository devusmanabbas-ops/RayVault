#ifndef RV_NORMALIZE_H
#define RV_NORMALIZE_H

#include "rv_parser.h"

/*
 * Normalization pass applied after parse: sort stable ids, clamp
 * severity, fill missing wave window back-links when unambiguous.
 */

typedef struct rv_normalize_stats {
    uint32_t sorted_windows;
    uint32_t clamped_severity;
    uint32_t filled_wave_links;
    uint32_t dropped_zero_routes;
} rv_normalize_stats;

int rv_normalize_parsed(rv_parsed *p, rv_normalize_stats *st);

#endif /* RV_NORMALIZE_H */
