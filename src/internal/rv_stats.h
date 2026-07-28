#ifndef RV_STATS_H
#define RV_STATS_H

#include "rayvault/rayvault_types.h"
#include "rv_index.h"
#include "rv_parser.h"

int rv_stats_compute(const rv_parsed *p, const rv_index *idx,
                     rv_stats_snapshot *out);

/* Derived summary retained across rebuilds until explicitly refreshed. */
typedef struct rv_derived_summary {
    rv_stats_snapshot snap;
    const char **route_labels; /* borrowed name pointers */
    uint32_t label_count;
    uint32_t index_generation;
    int      valid;
} rv_derived_summary;

int  rv_derived_summary_build(rv_derived_summary *ds, rv_arena *a,
                              const rv_parsed *p, const rv_index *idx);
void rv_derived_summary_invalidate(rv_derived_summary *ds);

#endif /* RV_STATS_H */
