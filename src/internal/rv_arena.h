#ifndef RV_ARENA_H
#define RV_ARENA_H

#include <stddef.h>
#include <stdint.h>

/*
 * Scratch arena used throughout RayVault for transient parse tables,
 * interned strings, and derived index nodes.
 *
 * Contract: pointers returned by rv_arena_alloc remain valid until
 * rv_arena_reset or rv_arena_destroy. Callers may retain interior
 * pointers across subsequent allocations within the same epoch.
 */
typedef struct rv_arena {
    uint8_t *base;
    size_t   size;
    size_t   used;
    size_t   peak;
    size_t   max_bytes;
    uint32_t epoch;      /* bumped on reset */
    uint32_t grow_count;
    int      owns_base;
} rv_arena;

int  rv_arena_init(rv_arena *a, size_t initial, size_t max_bytes);
void rv_arena_destroy(rv_arena *a);
void rv_arena_reset(rv_arena *a);

void *rv_arena_alloc(rv_arena *a, size_t nbytes, size_t align);
void *rv_arena_calloc(rv_arena *a, size_t count, size_t size, size_t align);
char *rv_arena_strdup(rv_arena *a, const char *s, size_t n);

size_t   rv_arena_used(const rv_arena *a);
size_t   rv_arena_avail(const rv_arena *a);
uint32_t rv_arena_epoch(const rv_arena *a);

/* Mark a watermark; later rewind frees everything after the mark without
 * bumping the epoch (pointers before the mark stay valid). */
typedef struct rv_arena_mark {
    size_t used;
} rv_arena_mark;

rv_arena_mark rv_arena_mark_get(const rv_arena *a);
void          rv_arena_rewind(rv_arena *a, rv_arena_mark m);

#endif /* RV_ARENA_H */
