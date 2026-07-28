#ifndef RV_EVENTS_H
#define RV_EVENTS_H

#include "rv_arena.h"
#include "rv_index.h"
#include "rv_parser.h"

#include <stddef.h>
#include <stdint.h>

/* Marker kind taxonomy used across monitoring dashboards. */
enum {
    RV_EVT_UNKNOWN = 0,
    RV_EVT_SPLICE = 1,
    RV_EVT_REFLECT = 2,
    RV_EVT_OPEN = 3,
    RV_EVT_BEND = 4,
    RV_EVT_GAIN = 5,
    RV_EVT_END = 6,
    RV_EVT_GHOST = 7
};

typedef struct rv_event_class {
    uint32_t marker_id;
    uint16_t kind;
    uint16_t severity;
    float    distance_m;
    float    loss_db;
    float    reflectance_db;
    int      is_actionable;
    int      is_ghost_candidate;
    const char *label;
} rv_event_class;

typedef struct rv_event_summary {
    uint32_t by_kind[8];
    uint32_t actionable;
    uint32_t ghosts;
    uint32_t critical;
} rv_event_summary;

const char *rv_event_kind_name(uint16_t kind);
int rv_event_classify_marker(const rv_marker_index_node *m, rv_event_class *out);
int rv_event_classify_all(rv_arena *a, const rv_index *idx,
                          rv_event_class **out, size_t *count);
int rv_event_summarize(const rv_index *idx, rv_event_summary *out);
int rv_event_filter_actionable(const rv_event_class *in, size_t n,
                               rv_event_class *out, size_t cap, size_t *written);
int rv_event_detect_ghosts(const rv_parsed *p, const rv_index *idx,
                           uint32_t *out_ids, size_t cap, size_t *written);

#endif /* RV_EVENTS_H */
