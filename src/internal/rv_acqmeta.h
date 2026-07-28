#ifndef RV_ACQMETA_H
#define RV_ACQMETA_H

#include "rv_parser.h"

#include <stdint.h>

/*
 * Acquisition metadata helpers derived from WIND + INST + CLBR triples.
 * Used by monitoring UIs to present per-span capture context.
 */

typedef struct rv_acq_context {
    uint32_t window_id;
    uint32_t route_id;
    uint32_t inst_id;
    uint32_t calib_id;
    uint16_t wavelength_nm;
    float    pulse_ns;
    float    range_km;
    float    resolution_m;
    float    refractive_index;
    uint64_t acquired_unix;
    uint64_t calibrated_unix;
    int      calib_stale; /* acquired after calib by > 1 year */
} rv_acq_context;

int rv_acq_context_for_window(const rv_parsed *p, uint32_t window_id,
                              rv_acq_context *out);
int rv_acq_list_stale_calibrations(const rv_parsed *p, uint32_t *out_inst,
                                   size_t cap, size_t *written);
int rv_acq_group_windows_by_day(const rv_parsed *p, uint64_t *day_keys,
                                uint32_t *counts, size_t cap, size_t *written);
uint64_t rv_acq_span_duration_sec(const rv_parsed *p, uint32_t route_id);

#endif /* RV_ACQMETA_H */
