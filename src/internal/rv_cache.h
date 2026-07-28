#ifndef RV_CACHE_H
#define RV_CACHE_H

#include "rv_arena.h"
#include "rv_index.h"
#include "rv_parser.h"

#include <stdint.h>

/*
 * LRU wave-block cache. Entries borrow sample slices from parsed WAVE
 * tables (which themselves borrow package backing). Generation counters
 * track invalidation across reopen / rebuild.
 */

typedef struct rv_cache_entry {
    uint32_t block_id;
    uint32_t window_id;
    const int16_t *samples; /* borrowed */
    uint32_t sample_count;
    float    start_m;
    float    step_m;
    uint32_t generation;
    uint64_t last_tick;
    int      in_use;
    int      pinned;
} rv_cache_entry;

typedef struct rv_wave_cache {
    rv_cache_entry *entries;
    size_t          capacity;
    size_t          count;
    uint64_t        tick;
    uint32_t        generation;
    uint64_t        hits;
    uint64_t        misses;
    uint64_t        evictions;
} rv_wave_cache;

int  rv_cache_init(rv_wave_cache *c, size_t capacity);
void rv_cache_destroy(rv_wave_cache *c);
void rv_cache_invalidate(rv_wave_cache *c);
int  rv_cache_get(rv_wave_cache *c, uint32_t block_id, rv_cache_entry **out);
int  rv_cache_put_from_block(rv_wave_cache *c, const rv_wave_block *b);
int  rv_cache_pin(rv_wave_cache *c, uint32_t block_id);
int  rv_cache_unpin(rv_wave_cache *c, uint32_t block_id);
int  rv_cache_evict_lru(rv_wave_cache *c);
size_t rv_cache_count(const rv_wave_cache *c);

#endif /* RV_CACHE_H */
