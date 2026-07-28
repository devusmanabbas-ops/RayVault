#ifndef RV_INDEX_H
#define RV_INDEX_H

#include "rv_arena.h"
#include "rv_parser.h"

/*
 * Secondary indexes over parsed tables. Index nodes store pointers into
 * arena-allocated parse tables and (for labels) into interned name strings.
 */

typedef struct rv_window_index_node {
    uint32_t window_id;
    uint32_t route_id;
    uint32_t inst_id;
    uint32_t table_index;
    const rv_window_rec *rec; /* borrowed from parse tables */
} rv_window_index_node;

typedef struct rv_marker_index_node {
    uint32_t marker_id;
    uint32_t window_id;
    uint16_t kind;
    uint16_t severity;
    float    distance_m;
    const char *label; /* borrowed interned string */
    const rv_marker_rec *rec;
} rv_marker_index_node;

typedef struct rv_wave_index_node {
    uint32_t block_id;
    uint32_t window_id;
    uint32_t table_index;
    const rv_wave_block *block;
} rv_wave_index_node;

typedef struct rv_route_index_node {
    uint32_t route_id;
    uint32_t table_index;
    const char *site_name;
    const rv_route_rec *rec;
} rv_route_index_node;

typedef struct rv_index {
    rv_window_index_node *windows;
    uint32_t window_count;
    rv_marker_index_node *markers;
    uint32_t marker_count;
    rv_wave_index_node *waves;
    uint32_t wave_count;
    rv_route_index_node *routes;
    uint32_t route_count;

    /* id -> index+1 maps */
    uint32_t *window_by_id;
    uint32_t  window_map_cap;
    uint32_t *wave_by_id;
    uint32_t  wave_map_cap;
    uint32_t *route_by_id;
    uint32_t  route_map_cap;

    uint32_t generation;
    int      ready;
} rv_index;

int  rv_index_build(rv_index *idx, rv_arena *a, const rv_parsed *parsed);
void rv_index_clear(rv_index *idx);
int  rv_index_find_window(const rv_index *idx, uint32_t window_id,
                          uint32_t *out_pos);
int  rv_index_find_wave(const rv_index *idx, uint32_t block_id,
                        uint32_t *out_pos);
int  rv_index_find_route(const rv_index *idx, uint32_t route_id,
                         uint32_t *out_pos);
int  rv_index_query_markers(const rv_index *idx, const rv_query_filter *flt,
                            rv_marker_info *out, size_t cap, size_t *written);

#endif /* RV_INDEX_H */
