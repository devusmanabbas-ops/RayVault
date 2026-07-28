#ifndef RV_TEXTEXPORT_H
#define RV_TEXTEXPORT_H

#include "rv_buf.h"
#include "rv_index.h"
#include "rv_parser.h"
#include "rv_analytics.h"
#include "rv_events.h"

int rv_textexport_routes_csv(rv_buf *out, const rv_parsed *p,
                             const rv_index *idx);
int rv_textexport_markers_csv(rv_buf *out, const rv_index *idx);
int rv_textexport_summary_txt(rv_buf *out, const rv_parsed *p,
                              const rv_plant_report *plant,
                              const rv_event_summary *ev);
int rv_textexport_windows_csv(rv_buf *out, const rv_parsed *p);

#endif /* RV_TEXTEXPORT_H */
