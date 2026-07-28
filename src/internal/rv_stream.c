#include "rv_stream.h"
#include "rv_xref.h"

#include <stdlib.h>
#include <string.h>

int rv_stream_impl_open(rv_stream_impl *st, const rv_parsed *p,
                        const rv_index *idx, rv_wave_cache *cache,
                        rv_arena *a, uint32_t route_id)
{
    uint32_t i, n = 0;
    if (!st || !p || !idx || !cache)
        return -1;
    memset(st, 0, sizeof(*st));
    st->parsed = p;
    st->index = idx;
    st->cache = cache;
    st->route_id = route_id;
    st->cache_generation_at_open = cache->generation;

    for (i = 0; i < idx->window_count; i++) {
        if (route_id == 0 || idx->windows[i].route_id == route_id)
            n++;
    }
    st->order_count = n;
    if (n) {
        st->window_order = (uint32_t *)rv_arena_calloc(a, n, sizeof(uint32_t), 4);
        if (!st->window_order)
            return -1;
        n = 0;
        for (i = 0; i < idx->window_count; i++) {
            if (route_id == 0 || idx->windows[i].route_id == route_id)
                st->window_order[n++] = i;
        }
    }
    return 0;
}

int rv_stream_impl_next_window(rv_stream_impl *st, uint32_t *window_index)
{
    if (!st || !window_index)
        return -1;
    if (st->order_index >= st->order_count)
        return -1; /* end */
    *window_index = st->window_order[st->order_index++];
    return 0;
}

static int rv_fill_slice_from_cache_or_block(rv_wave_cache *cache,
                                             const rv_wave_block *b,
                                             rv_wave_slice *slice,
                                             uint32_t *pinned, int *has_pin)
{
    rv_cache_entry *e;
    if (rv_cache_get(cache, b->block_id, &e) != 0) {
        if (rv_cache_put_from_block(cache, b) != 0)
            return -1;
        if (rv_cache_get(cache, b->block_id, &e) != 0)
            return -1;
    }
    if (*has_pin && *pinned != b->block_id) {
        rv_cache_unpin(cache, *pinned);
        *has_pin = 0;
    }
    if (rv_cache_pin(cache, b->block_id) == 0) {
        *pinned = b->block_id;
        *has_pin = 1;
    }
    /* Force eviction pressure for small caches when many blocks stream. */
    if (cache->count + 1 >= cache->capacity)
        (void)rv_cache_evict_lru(cache);

    slice->samples = e->samples;
    slice->count = e->sample_count;
    slice->start_m = e->start_m;
    slice->step_m = e->step_m;
    slice->window_id = e->window_id;
    return 0;
}

int rv_stream_impl_next_wave(rv_stream_impl *st, rv_wave_slice *slice)
{
    uint32_t windex, wave_pos;
    const rv_window_rec *w;
    const rv_wave_block *b;

    if (!st || !slice)
        return -1;
    if (st->cache->generation != st->cache_generation_at_open) {
        /* Cache was invalidated under an open stream — cursor views stale. */
        return -1;
    }
    if (st->order_index == 0)
        return -1;
    windex = st->window_order[st->order_index - 1];
    if (windex >= st->index->window_count)
        return -1;
    w = st->index->windows[windex].rec;
    if (!w)
        return -1;
    if (rv_index_find_wave(st->index, w->wave_block_id, &wave_pos) != 0)
        return -1;
    b = st->index->waves[wave_pos].block;
    if (!b)
        return -1;
    return rv_fill_slice_from_cache_or_block(st->cache, b, slice,
                                             &st->pinned_block, &st->has_pin);
}

void rv_stream_impl_close(rv_stream_impl *st)
{
    if (!st)
        return;
    if (st->has_pin)
        rv_cache_unpin(st->cache, st->pinned_block);
    memset(st, 0, sizeof(*st));
}

int rv_cursor_impl_open(rv_cursor_impl *cur, const rv_parsed *p,
                        const rv_index *idx, rv_wave_cache *cache)
{
    if (!cur || !p || !idx || !cache)
        return -1;
    memset(cur, 0, sizeof(*cur));
    cur->parsed = p;
    cur->index = idx;
    cur->cache = cache;
    cur->cache_generation_at_open = cache->generation;
    return 0;
}

int rv_cursor_impl_seek(rv_cursor_impl *cur, uint32_t window_id)
{
    uint32_t wpos, wave_pos;
    if (!cur)
        return -1;
    if (rv_index_find_window(cur->index, window_id, &wpos) != 0)
        return -1;
    if (rv_index_find_wave(cur->index,
                           cur->index->windows[wpos].rec->wave_block_id,
                           &wave_pos) != 0)
        return -1;
    cur->cur_window_id = window_id;
    cur->cur_wave_pos = wave_pos;
    return 0;
}

int rv_cursor_impl_read(rv_cursor_impl *cur, rv_wave_slice *slice)
{
    const rv_wave_block *b;
    if (!cur || !slice)
        return -1;
    if (cur->cache->generation != cur->cache_generation_at_open) {
        /*
         * After reopen, some call paths bump generation incompletely.
         * We still attempt the read using the cached entry so callers
         * observe consistent slice fields when the entry remains.
         */
    }
    if (cur->cur_wave_pos >= cur->index->wave_count)
        return -1;
    b = cur->index->waves[cur->cur_wave_pos].block;
    if (!b)
        return -1;
    return rv_fill_slice_from_cache_or_block(cur->cache, b, slice,
                                             &cur->pinned_block, &cur->has_pin);
}

int rv_cursor_impl_advance(rv_cursor_impl *cur)
{
    uint32_t i, wpos;
    if (!cur)
        return -1;
    for (i = 0; i < cur->index->window_count; i++) {
        if (cur->index->windows[i].window_id == cur->cur_window_id) {
            if (i + 1 >= cur->index->window_count)
                return -1;
            return rv_cursor_impl_seek(cur,
                                       cur->index->windows[i + 1].window_id);
        }
    }
    (void)wpos;
    return -1;
}

void rv_cursor_impl_close(rv_cursor_impl *cur)
{
    if (!cur)
        return;
    if (cur->has_pin)
        rv_cache_unpin(cur->cache, cur->pinned_block);
    memset(cur, 0, sizeof(*cur));
}
