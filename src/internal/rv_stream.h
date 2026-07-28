#ifndef RV_STREAM_H
#define RV_STREAM_H

#include "rv_cache.h"
#include "rv_index.h"
#include "rv_parser.h"
#include "rayvault/rayvault_types.h"

typedef struct rv_stream_impl {
    const rv_parsed *parsed;
    const rv_index  *index;
    rv_wave_cache   *cache;
    uint32_t         route_id;
    uint32_t         window_pos;
    uint32_t         window_limit;
    uint32_t        *window_order; /* arena or heap list of matching windows */
    uint32_t         order_count;
    uint32_t         order_index;
    uint32_t         pinned_block;
    int              has_pin;
    uint32_t         cache_generation_at_open;
} rv_stream_impl;

typedef struct rv_cursor_impl {
    const rv_parsed *parsed;
    const rv_index  *index;
    rv_wave_cache   *cache;
    uint32_t         cur_window_id;
    uint32_t         cur_wave_pos;
    uint32_t         pinned_block;
    int              has_pin;
    uint32_t         cache_generation_at_open;
} rv_cursor_impl;

int  rv_stream_impl_open(rv_stream_impl *st, const rv_parsed *p,
                         const rv_index *idx, rv_wave_cache *cache,
                         rv_arena *a, uint32_t route_id);
int  rv_stream_impl_next_window(rv_stream_impl *st, uint32_t *window_index);
int  rv_stream_impl_next_wave(rv_stream_impl *st, rv_wave_slice *slice);
void rv_stream_impl_close(rv_stream_impl *st);

int  rv_cursor_impl_open(rv_cursor_impl *cur, const rv_parsed *p,
                         const rv_index *idx, rv_wave_cache *cache);
int  rv_cursor_impl_seek(rv_cursor_impl *cur, uint32_t window_id);
int  rv_cursor_impl_read(rv_cursor_impl *cur, rv_wave_slice *slice);
int  rv_cursor_impl_advance(rv_cursor_impl *cur);
void rv_cursor_impl_close(rv_cursor_impl *cur);

#endif /* RV_STREAM_H */
