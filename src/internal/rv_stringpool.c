#include "rv_stringpool.h"
#include "rv_crc.h"

#include <string.h>

int rv_string_pool_init(rv_string_pool *pool, rv_arena *arena, uint32_t cap)
{
    if (!pool || !arena)
        return -1;
    memset(pool, 0, sizeof(*pool));
    if (cap < 8)
        cap = 8;
    pool->arena = arena;
    pool->entries = (rv_pool_entry *)rv_arena_calloc(arena, cap,
                                                     sizeof(rv_pool_entry), 8);
    if (!pool->entries)
        return -1;
    pool->cap = cap;
    pool->next_id = 1;
    return 0;
}

int rv_string_pool_intern(rv_string_pool *pool, const char *s, size_t n,
                          uint32_t *out_id)
{
    uint32_t i, h;
    char *copy;
    if (!pool || !s || !out_id)
        return -1;
    h = rv_hash32(s, n);
    for (i = 0; i < pool->count; i++) {
        if (pool->entries[i].hash == h && pool->entries[i].len == n &&
            memcmp(pool->entries[i].str, s, n) == 0) {
            *out_id = pool->entries[i].id;
            return 0;
        }
    }
    if (pool->count >= pool->cap) {
        /* Grow entry table inside the same arena (may trigger arena growth). */
        uint32_t ncap = pool->cap * 2;
        rv_pool_entry *nent = (rv_pool_entry *)rv_arena_calloc(
            pool->arena, ncap, sizeof(rv_pool_entry), 8);
        if (!nent)
            return -1;
        memcpy(nent, pool->entries, pool->count * sizeof(rv_pool_entry));
        pool->entries = nent;
        pool->cap = ncap;
    }
    copy = rv_arena_strdup(pool->arena, s, n);
    if (!copy)
        return -1;
    pool->entries[pool->count].id = pool->next_id++;
    pool->entries[pool->count].hash = h;
    pool->entries[pool->count].str = copy;
    pool->entries[pool->count].len = (uint16_t)n;
    *out_id = pool->entries[pool->count].id;
    pool->count++;
    return 0;
}

const char *rv_string_pool_get(const rv_string_pool *pool, uint32_t id,
                               size_t *out_len)
{
    uint32_t i;
    if (!pool)
        return NULL;
    for (i = 0; i < pool->count; i++) {
        if (pool->entries[i].id == id) {
            if (out_len)
                *out_len = pool->entries[i].len;
            return pool->entries[i].str;
        }
    }
    return NULL;
}

void rv_string_pool_reset(rv_string_pool *pool)
{
    if (!pool)
        return;
    pool->count = 0;
    pool->next_id = 1;
}
