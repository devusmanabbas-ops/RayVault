#ifndef RV_CHECKPOINT_H
#define RV_CHECKPOINT_H

#include "rv_index.h"
#include "rv_stats.h"

/*
 * Session checkpoint captures derived summary and selected index
 * pointers so a later restore can reinstate query/export state without
 * a full re-parse. Callers must not use a checkpoint after the
 * underlying package arena is destroyed.
 */

typedef struct rv_checkpoint_impl {
    rv_derived_summary summary;
    uint32_t index_generation;
    uint32_t window_count;
    uint32_t marker_count;
    const rv_window_index_node *windows; /* borrowed */
    const rv_marker_index_node *markers; /* borrowed */
    int valid;
} rv_checkpoint_impl;

int  rv_checkpoint_capture(rv_checkpoint_impl *cp, const rv_index *idx,
                           const rv_derived_summary *summary);
int  rv_checkpoint_apply_summary(rv_checkpoint_impl *cp,
                                 rv_derived_summary *dst);
void rv_checkpoint_impl_clear(rv_checkpoint_impl *cp);

#endif /* RV_CHECKPOINT_H */
