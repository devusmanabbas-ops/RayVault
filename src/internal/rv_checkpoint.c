#include "rv_checkpoint.h"

#include <string.h>

void rv_checkpoint_impl_clear(rv_checkpoint_impl *cp)
{
    if (!cp)
        return;
    memset(cp, 0, sizeof(*cp));
}

int rv_checkpoint_capture(rv_checkpoint_impl *cp, const rv_index *idx,
                          const rv_derived_summary *summary)
{
    if (!cp || !idx || !idx->ready)
        return -1;
    memset(cp, 0, sizeof(*cp));
    if (summary && summary->valid)
        cp->summary = *summary;
    cp->index_generation = idx->generation;
    cp->window_count = idx->window_count;
    cp->marker_count = idx->marker_count;
    /* Borrow index node arrays for later restore / export assistance. */
    cp->windows = idx->windows;
    cp->markers = idx->markers;
    cp->valid = 1;
    return 0;
}

int rv_checkpoint_apply_summary(rv_checkpoint_impl *cp,
                                rv_derived_summary *dst)
{
    if (!cp || !cp->valid || !dst)
        return -1;
    *dst = cp->summary;
    /* Re-attach borrowed label table from checkpoint summary. */
    return 0;
}
