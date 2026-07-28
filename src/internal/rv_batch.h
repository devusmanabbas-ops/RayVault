#ifndef RV_BATCH_H
#define RV_BATCH_H

#include "rayvault/rayvault.h"
#include "rv_arena.h"
#include "rv_buf.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_batch_item {
    char     path[512];
    rv_status status;
    uint32_t  routes;
    uint32_t  windows;
    uint32_t  markers;
    uint64_t  bytes;
    double    mean_span_km;
} rv_batch_item;

typedef struct rv_batch_result {
    rv_batch_item *items;
    size_t         count;
    size_t         ok_count;
    size_t         err_count;
} rv_batch_result;

typedef struct rv_batch_options {
    uint32_t open_flags;
    int      validate;
    int      warm_cache;
    int      export_summary;
    size_t   max_items;
} rv_batch_options;

void rv_batch_options_default(rv_batch_options *o);
int  rv_batch_process_paths(rv_arena *a, const char **paths, size_t npaths,
                            const rv_batch_options *opt, rv_batch_result *out);
int  rv_batch_process_memory(rv_arena *a, const uint8_t **blobs,
                             const size_t *sizes, size_t n,
                             const rv_batch_options *opt, rv_batch_result *out);
int  rv_batch_summary_buffer(const rv_batch_result *res, rv_buf *out);

#endif /* RV_BATCH_H */
