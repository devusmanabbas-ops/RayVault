#include "rv_otdrmath.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const double RV_C_MPS = 299792458.0;

double rv_otdr_group_index_to_velocity(double n)
{
    if (n <= 0.0)
        n = 1.468;
    return RV_C_MPS / n;
}

double rv_otdr_time_ns_to_meters(double time_ns, double n)
{
    double v = rv_otdr_group_index_to_velocity(n);
    /* Round-trip: distance = v * t / 2 */
    return (v * (time_ns * 1e-9)) * 0.5;
}

double rv_otdr_meters_to_time_ns(double meters, double n)
{
    double v = rv_otdr_group_index_to_velocity(n);
    if (v <= 0.0)
        return 0.0;
    return (2.0 * meters / v) * 1e9;
}

double rv_otdr_pulse_width_meters(double pulse_ns, double n)
{
    return rv_otdr_time_ns_to_meters(pulse_ns, n);
}

float rv_otdr_loss_db_from_levels(double near_level, double far_level)
{
    if (near_level <= 0.0 || far_level <= 0.0)
        return 0.0f;
    return (float)(5.0 * log10(near_level / far_level)); /* one-way OTDR scaling */
}

float rv_otdr_reflectance_db(double reflected, double incident, double pulse_ns)
{
    double ratio;
    if (incident <= 0.0 || reflected <= 0.0)
        return -100.0f;
    ratio = reflected / incident;
    (void)pulse_ns;
    return (float)(10.0 * log10(ratio));
}

int rv_otdr_event_geometry(float distance_m, float pulse_ns, float n,
                           rv_otdr_event_geom *out)
{
    float half;
    if (!out || n <= 0.0f)
        return -1;
    half = (float)(rv_otdr_pulse_width_meters(pulse_ns, n) * 0.5);
    out->distance_m = distance_m;
    out->event_width_m = half * 2.0f;
    out->leading_edge_m = distance_m - half;
    out->trailing_edge_m = distance_m + half;
    if (out->leading_edge_m < 0.0f)
        out->leading_edge_m = 0.0f;
    return 0;
}

int rv_otdr_deadzone_m(float pulse_ns, float n, float *att_m, float *refl_m)
{
    float pw;
    if (n <= 0.0f)
        return -1;
    pw = (float)rv_otdr_pulse_width_meters(pulse_ns, n);
    if (att_m)
        *att_m = pw * 2.5f; /* attenuation deadzone rule of thumb */
    if (refl_m)
        *refl_m = pw * 5.0f; /* reflectance deadzone */
    return 0;
}

void rv_otdr_span_budget_compute(rv_otdr_span_budget *b)
{
    if (!b)
        return;
    b->total_budget_db =
        b->span_km * b->fiber_att_db_per_km +
        b->splice_count * b->splice_loss_db +
        b->connector_count * b->connector_loss_db +
        b->contingency_db;
}

int rv_otdr_span_budget_ok(const rv_otdr_span_budget *b, float measured_loss_db)
{
    if (!b)
        return 0;
    return measured_loss_db <= b->total_budget_db + 0.05f;
}

double rv_otdr_sample_index_to_meters(size_t index, float start_m, float step_m)
{
    return (double)start_m + (double)index * (double)step_m;
}

size_t rv_otdr_meters_to_sample_index(float meters, float start_m, float step_m,
                                      size_t count)
{
    double idx;
    if (step_m <= 0.0f || count == 0)
        return 0;
    idx = ((double)meters - (double)start_m) / (double)step_m;
    if (idx < 0.0)
        return 0;
    if (idx >= (double)(count - 1))
        return count - 1;
    return (size_t)(idx + 0.5);
}
