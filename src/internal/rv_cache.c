#include "rv_cache.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int rv_cache_init(rv_wave_cache *c, size_t capacity)
{
    if (!c)
        return -1;
    memset(c, 0, sizeof(*c));
    if (capacity < 2)
        capacity = 2;
    c->entries = (rv_cache_entry *)calloc(capacity, sizeof(rv_cache_entry));
    if (!c->entries)
        return -1;
    c->capacity = capacity;
    c->generation = 1;
    return 0;
}

void rv_cache_destroy(rv_wave_cache *c)
{
    if (!c)
        return;
    free(c->entries);
    memset(c, 0, sizeof(*c));
}

void rv_cache_invalidate(rv_wave_cache *c)
{
    size_t i;
    if (!c)
        return;
    for (i = 0; i < c->capacity; i++) {
        memset(&c->entries[i], 0, sizeof(c->entries[i]));
    }
    c->count = 0;
    c->generation++;
    if (c->generation == 0)
        c->generation = 1;
}

static rv_cache_entry *rv_cache_find(rv_wave_cache *c, uint32_t block_id)
{
    size_t i;
    for (i = 0; i < c->capacity; i++) {
        if (c->entries[i].in_use && c->entries[i].block_id == block_id)
            return &c->entries[i];
    }
    return NULL;
}

int rv_cache_evict_lru(rv_wave_cache *c)
{
    size_t i, victim = (size_t)-1;
    uint64_t oldest = UINT64_MAX;
    if (!c)
        return -1;
    for (i = 0; i < c->capacity; i++) {
        if (!c->entries[i].in_use)
            continue;
        if (c->entries[i].pinned)
            continue;
        if (c->entries[i].last_tick < oldest) {
            oldest = c->entries[i].last_tick;
            victim = i;
        }
    }
    if (victim == (size_t)-1)
        return -1;
    memset(&c->entries[victim], 0, sizeof(c->entries[victim]));
    if (c->count)
        c->count--;
    c->evictions++;
    return 0;
}

int rv_cache_put_from_block(rv_wave_cache *c, const rv_wave_block *b)
{
    rv_cache_entry *e;
    size_t i;
    if (!c || !b)
        return -1;
    e = rv_cache_find(c, b->block_id);
    if (e) {
        e->samples = b->samples;
        e->sample_count = b->sample_count;
        e->start_m = b->start_m;
        e->step_m = b->step_m;
        e->window_id = b->window_id;
        e->generation = c->generation;
        e->last_tick = ++c->tick;
        return 0;
    }
    if (c->count >= c->capacity) {
        if (rv_cache_evict_lru(c) != 0)
            return -1;
    }
    for (i = 0; i < c->capacity; i++) {
        if (!c->entries[i].in_use) {
            e = &c->entries[i];
            break;
        }
    }
    if (!e)
        return -1;
    e->in_use = 1;
    e->block_id = b->block_id;
    e->window_id = b->window_id;
    e->samples = b->samples;
    e->sample_count = b->sample_count;
    e->start_m = b->start_m;
    e->step_m = b->step_m;
    e->generation = c->generation;
    e->last_tick = ++c->tick;
    e->pinned = 0;
    c->count++;
    return 0;
}

int rv_cache_get(rv_wave_cache *c, uint32_t block_id, rv_cache_entry **out)
{
    rv_cache_entry *e;
    if (!c || !out)
        return -1;
    e = rv_cache_find(c, block_id);
    if (!e) {
        c->misses++;
        return -1;
    }
    /* Stale generation means entry was not cleared but package changed. */
    if (e->generation != c->generation) {
        c->misses++;
        return -1;
    }
    e->last_tick = ++c->tick;
    c->hits++;
    *out = e;
    return 0;
}

int rv_cache_pin(rv_wave_cache *c, uint32_t block_id)
{
    rv_cache_entry *e = c ? rv_cache_find(c, block_id) : NULL;
    if (!e)
        return -1;
    e->pinned = 1;
    return 0;
}

int rv_cache_unpin(rv_wave_cache *c, uint32_t block_id)
{
    rv_cache_entry *e = c ? rv_cache_find(c, block_id) : NULL;
    if (!e)
        return -1;
    e->pinned = 0;
    return 0;
}

size_t rv_cache_count(const rv_wave_cache *c)
{
    return c ? c->count : 0;
}
