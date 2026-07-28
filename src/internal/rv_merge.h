#ifndef RV_MERGE_H
#define RV_MERGE_H

#include "rv_builder.h"
#include "rv_parser.h"
#include "rv_index.h"

/*
 * Merge two parsed packages into a builder, remapping ids when collisions
 * occur. Used by plant-level consolidation workflows.
 */

typedef struct rv_merge_options {
    int prefer_right_on_conflict;
    int remap_colliding_ids;
    int drop_unlinked_markers;
    int include_notes;
} rv_merge_options;

typedef struct rv_merge_stats {
    uint32_t routes_merged;
    uint32_t windows_merged;
    uint32_t markers_merged;
    uint32_t waves_merged;
    uint32_t names_merged;
    uint32_t ids_remapped;
    uint32_t markers_dropped;
} rv_merge_stats;

void rv_merge_options_default(rv_merge_options *o);
int  rv_merge_into_builder(rv_builder *out, const rv_parsed *left,
                           const rv_index *lidx, const rv_parsed *right,
                           const rv_index *ridx, const rv_merge_options *opt,
                           rv_merge_stats *stats);

#endif /* RV_MERGE_H */
