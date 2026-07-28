#ifndef RV_OTDRMATH_H
#define RV_OTDRMATH_H

#include <stddef.h>
#include <stdint.h>

/* Physical helpers used when converting between OTDR timebases and fiber
 * distance using calibration refractive indices. */

double rv_otdr_group_index_to_velocity(double n);
double rv_otdr_time_ns_to_meters(double time_ns, double n);
double rv_otdr_meters_to_time_ns(double meters, double n);
double rv_otdr_pulse_width_meters(double pulse_ns, double n);
float  rv_otdr_loss_db_from_levels(double near_level, double far_level);
float  rv_otdr_reflectance_db(double reflected, double incident, double pulse_ns);

typedef struct rv_otdr_event_geom {
    float distance_m;
    float event_width_m;
    float leading_edge_m;
    float trailing_edge_m;
} rv_otdr_event_geom;

int rv_otdr_event_geometry(float distance_m, float pulse_ns, float n,
                           rv_otdr_event_geom *out);
int rv_otdr_deadzone_m(float pulse_ns, float n, float *att_m, float *refl_m);

typedef struct rv_otdr_span_budget {
    float span_km;
    float fiber_att_db_per_km;
    float splice_count;
    float splice_loss_db;
    float connector_count;
    float connector_loss_db;
    float contingency_db;
    float total_budget_db;
} rv_otdr_span_budget;

void rv_otdr_span_budget_compute(rv_otdr_span_budget *b);
int  rv_otdr_span_budget_ok(const rv_otdr_span_budget *b, float measured_loss_db);

double rv_otdr_sample_index_to_meters(size_t index, float start_m, float step_m);
size_t rv_otdr_meters_to_sample_index(float meters, float start_m, float step_m,
                                      size_t count);

#endif /* RV_OTDRMATH_H */
