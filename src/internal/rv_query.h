#ifndef RV_QUERY_H
#define RV_QUERY_H

#include "rayvault/rayvault_types.h"
#include "rv_arena.h"
#include "rv_index.h"
#include "rv_parser.h"

typedef struct rv_query_plan {
    rv_query_filter filter;
    int             sort_by_distance;
    int             sort_by_severity;
    int             require_wave;
    uint32_t        limit;
} rv_query_plan;

typedef struct rv_query_result {
    rv_marker_info *markers;
    size_t          count;
    uint32_t       *window_ids;
    size_t          window_count;
} rv_query_result;

void rv_query_plan_init(rv_query_plan *plan);
int  rv_query_execute(rv_arena *a, const rv_parsed *p, const rv_index *idx,
                      const rv_query_plan *plan, rv_query_result *out);
int  rv_query_windows_for_route(const rv_index *idx, uint32_t route_id,
                                uint32_t *out, size_t cap, size_t *written);
int  rv_query_markers_near(const rv_index *idx, uint32_t window_id,
                           float center_m, float radius_m,
                           rv_marker_info *out, size_t cap, size_t *written);
int  rv_query_instruments_on_route(const rv_parsed *p, const rv_index *idx,
                                   uint32_t route_id, uint32_t *out,
                                   size_t cap, size_t *written);

#endif /* RV_QUERY_H */
