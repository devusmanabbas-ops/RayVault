#ifndef RV_REPORT_H
#define RV_REPORT_H

#include "rv_analytics.h"
#include "rv_events.h"
#include "rv_parser.h"
#include "rv_index.h"
#include "rv_buf.h"

/*
 * Text/JSON-ish report emitters for operator tooling. Output is appended
 * into an rv_buf for the caller to write or display.
 */

typedef struct rv_report_options {
    int include_routes;
    int include_events;
    int include_histogram;
    int include_wave_summary;
    int json_style;
} rv_report_options;

void rv_report_options_default(rv_report_options *o);
int  rv_report_render(rv_buf *out, const rv_parsed *p, const rv_index *idx,
                      const rv_plant_report *plant, const rv_event_summary *ev,
                      const rv_report_options *opt);
int  rv_report_render_route(rv_buf *out, const rv_route_health *rh);
int  rv_report_render_marker_line(rv_buf *out, const rv_event_class *ec);

#endif /* RV_REPORT_H */
