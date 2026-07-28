#ifndef RV_ANALYTICS_H
#define RV_ANALYTICS_H

#include "rv_index.h"
#include "rv_parser.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_loss_histogram {
    uint32_t buckets[16];
    float    edges[17];
    uint32_t total;
} rv_loss_histogram;

typedef struct rv_route_health {
    uint32_t route_id;
    uint32_t window_count;
    uint32_t marker_count;
    uint32_t critical_markers;
    float    max_loss_db;
    float    mean_loss_db;
    float    span_km;
    const char *site_label;
} rv_route_health;

typedef struct rv_plant_report {
    rv_route_health *routes;
    uint32_t         route_count;
    rv_loss_histogram hist;
    double           plant_km;
    uint32_t         open_events;
    uint32_t         splice_events;
    uint32_t         reflect_events;
} rv_plant_report;

int rv_analytics_loss_histogram(const rv_parsed *p, rv_loss_histogram *h);
int rv_analytics_route_health(rv_arena *a, const rv_parsed *p,
                              const rv_index *idx, rv_plant_report *out);
int rv_analytics_top_loss_markers(const rv_index *idx, rv_marker_info *out,
                                  size_t cap, size_t *written);
int rv_analytics_wavelength_coverage(const rv_parsed *p, uint16_t *waves,
                                     size_t cap, size_t *written);
double rv_analytics_estimate_attenuation(const rv_parsed *p, uint32_t route_id);

#endif /* RV_ANALYTICS_H */
