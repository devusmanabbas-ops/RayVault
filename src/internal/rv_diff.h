#ifndef RV_DIFF_H
#define RV_DIFF_H

#include "rv_arena.h"
#include "rv_parser.h"
#include "rv_index.h"

#include <stddef.h>
#include <stdint.h>

typedef enum rv_diff_kind {
    RV_DIFF_NONE = 0,
    RV_DIFF_ADDED,
    RV_DIFF_REMOVED,
    RV_DIFF_CHANGED
} rv_diff_kind;

typedef struct rv_diff_entry {
    rv_diff_kind kind;
    uint32_t     entity_kind; /* 1 route 2 window 3 marker 4 wave */
    uint32_t     id;
    float        delta_loss_db;
    float        delta_distance_m;
} rv_diff_entry;

typedef struct rv_diff_report {
    rv_diff_entry *entries;
    size_t         count;
    uint32_t       routes_added;
    uint32_t       routes_removed;
    uint32_t       markers_added;
    uint32_t       markers_removed;
    uint32_t       markers_changed;
} rv_diff_report;

int rv_diff_packages(rv_arena *a, const rv_parsed *left, const rv_index *lidx,
                     const rv_parsed *right, const rv_index *ridx,
                     rv_diff_report *out);
int rv_diff_marker_delta(const rv_index *left, const rv_index *right,
                         uint32_t marker_id, float *loss_delta);

#endif /* RV_DIFF_H */
