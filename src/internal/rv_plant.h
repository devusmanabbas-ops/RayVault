#ifndef RV_PLANT_H
#define RV_PLANT_H

#include "rv_arena.h"
#include "rv_index.h"
#include "rv_parser.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_plant_node {
    uint32_t route_id;
    uint32_t site_name_id;
    uint32_t far_name_id;
    float    length_km;
    uint32_t degree; /* number of linked windows */
    const char *site_label;
    const char *far_label;
} rv_plant_node;

typedef struct rv_plant_edge {
    uint32_t from_route;
    uint32_t to_route;
    uint32_t via_link_id;
    float    shared_hint_km;
} rv_plant_edge;

typedef struct rv_plant_graph {
    rv_plant_node *nodes;
    uint32_t       node_count;
    rv_plant_edge *edges;
    uint32_t       edge_count;
    double         total_km;
} rv_plant_graph;

int rv_plant_build_graph(rv_arena *a, const rv_parsed *p, const rv_index *idx,
                         rv_plant_graph *out);
int rv_plant_longest_path_km(const rv_plant_graph *g, double *out_km);
int rv_plant_find_hubs(const rv_plant_graph *g, uint32_t *out_route_ids,
                       size_t cap, size_t *written);
int rv_plant_orphan_routes(const rv_parsed *p, const rv_index *idx,
                           uint32_t *out, size_t cap, size_t *written);
int rv_plant_fiber_capacity(const rv_parsed *p, uint32_t *out_total_fibers);

#endif /* RV_PLANT_H */
