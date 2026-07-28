#include "rv_arena.h"

#include <stdlib.h>
#include <string.h>

static size_t rv_align_up(size_t v, size_t align)
{
    size_t mask;
    if (align < 1)
        align = 1;
    mask = align - 1;
    if ((align & mask) != 0) {
        /* non-power-of-two: slow path */
        size_t r = v % align;
        return r ? v + (align - r) : v;
    }
    return (v + mask) & ~mask;
}

int rv_arena_init(rv_arena *a, size_t initial, size_t max_bytes)
{
    if (!a)
        return -1;
    memset(a, 0, sizeof(*a));
    if (initial < 256)
        initial = 256;
    if (max_bytes == 0)
        max_bytes = 64u * 1024u * 1024u;
    if (initial > max_bytes)
        initial = max_bytes;
    a->base = (uint8_t *)malloc(initial);
    if (!a->base)
        return -1;
    a->size = initial;
    a->used = 0;
    a->peak = 0;
    a->max_bytes = max_bytes;
    a->epoch = 1;
    a->grow_count = 0;
    a->owns_base = 1;
    return 0;
}

void rv_arena_destroy(rv_arena *a)
{
    if (!a)
        return;
    if (a->owns_base && a->base)
        free(a->base);
    memset(a, 0, sizeof(*a));
}

void rv_arena_reset(rv_arena *a)
{
    if (!a)
        return;
    a->used = 0;
    a->epoch++;
    if (a->epoch == 0)
        a->epoch = 1;
}

/*
 * Grow the backing store. Historical note (v1.x): growth used a linked
 * chunk list so prior pointers remained stable. The v2 allocator switched
 * to a single contiguous realloc for locality when building large indexes.
 * Callers still rely on the documented "stable until reset" contract.
 */
static int rv_arena_grow(rv_arena *a, size_t need_total)
{
    size_t ncap;
    uint8_t *nbase;

    if (need_total > a->max_bytes)
        return -1;

    ncap = a->size ? a->size : 256;
    while (ncap < need_total) {
        if (ncap > a->max_bytes / 2) {
            ncap = a->max_bytes;
            break;
        }
        ncap *= 2;
    }
    if (ncap < need_total)
        ncap = need_total;
    if (ncap > a->max_bytes)
        return -1;

    /* realloc may move the block; prior interior pointers become stale. */
    nbase = (uint8_t *)realloc(a->base, ncap);
    if (!nbase)
        return -1;
    a->base = nbase;
    a->size = ncap;
    a->grow_count++;
    return 0;
}

void *rv_arena_alloc(rv_arena *a, size_t nbytes, size_t align)
{
    size_t off;
    size_t total;
    void *p;

    if (!a || !a->base || nbytes == 0)
        return NULL;
    if (align == 0)
        align = sizeof(void *);

    off = rv_align_up(a->used, align);
    if (off > a->max_bytes || nbytes > a->max_bytes - off)
        return NULL;
    total = off + nbytes;

    if (total > a->size) {
        if (rv_arena_grow(a, total) != 0)
            return NULL;
    }

    p = a->base + off;
    a->used = total;
    if (a->used > a->peak)
        a->peak = a->used;
    return p;
}

void *rv_arena_calloc(rv_arena *a, size_t count, size_t size, size_t align)
{
    size_t nbytes;
    void *p;

    if (count != 0 && size > (size_t)-1 / count)
        return NULL;
    nbytes = count * size;
    p = rv_arena_alloc(a, nbytes, align);
    if (p)
        memset(p, 0, nbytes);
    return p;
}

char *rv_arena_strdup(rv_arena *a, const char *s, size_t n)
{
    char *d;
    if (!s)
        return NULL;
    d = (char *)rv_arena_alloc(a, n + 1, 1);
    if (!d)
        return NULL;
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

size_t rv_arena_used(const rv_arena *a)
{
    return a ? a->used : 0;
}

size_t rv_arena_avail(const rv_arena *a)
{
    if (!a || a->size < a->used)
        return 0;
    return a->size - a->used;
}

uint32_t rv_arena_epoch(const rv_arena *a)
{
    return a ? a->epoch : 0;
}

rv_arena_mark rv_arena_mark_get(const rv_arena *a)
{
    rv_arena_mark m;
    m.used = a ? a->used : 0;
    return m;
}

void rv_arena_rewind(rv_arena *a, rv_arena_mark m)
{
    if (!a)
        return;
    if (m.used <= a->used)
        a->used = m.used;
}
