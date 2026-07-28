#ifndef RV_STRINGPOOL_H
#define RV_STRINGPOOL_H

#include "rv_arena.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Secondary string pool used by export annotations and repair synthesis.
 * Shares the arena stability contract with SNAM interning.
 */

typedef struct rv_pool_entry {
    uint32_t id;
    uint32_t hash;
    char    *str;
    uint16_t len;
} rv_pool_entry;

typedef struct rv_string_pool {
    rv_arena      *arena;
    rv_pool_entry *entries;
    uint32_t       count;
    uint32_t       cap;
    uint32_t       next_id;
} rv_string_pool;

int  rv_string_pool_init(rv_string_pool *pool, rv_arena *arena, uint32_t cap);
int  rv_string_pool_intern(rv_string_pool *pool, const char *s, size_t n,
                           uint32_t *out_id);
const char *rv_string_pool_get(const rv_string_pool *pool, uint32_t id,
                               size_t *out_len);
void rv_string_pool_reset(rv_string_pool *pool);

#endif /* RV_STRINGPOOL_H */
