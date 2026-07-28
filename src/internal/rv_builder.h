#ifndef RV_BUILDER_H
#define RV_BUILDER_H

#include "rv_buf.h"
#include "rv_arena.h"
#include "rayvault/rayvault_types.h"

#include <stddef.h>
#include <stdint.h>

/*
 * In-memory RVP package builder used by acquisition exporters and the
 * offline packaging utility. Mirrors the on-disk section layout.
 */

typedef struct rv_builder_name {
    uint32_t id;
    char     str[128];
    uint16_t len;
} rv_builder_name;

typedef struct rv_builder_route {
    uint32_t route_id;
    uint32_t site_name_id;
    uint32_t far_name_id;
    float    length_km;
    uint32_t fiber_count;
    uint32_t flags;
} rv_builder_route;

typedef struct rv_builder_inst {
    uint32_t inst_id;
    uint32_t model_name_id;
    uint32_t serial_name_id;
    uint16_t wavelength_nm;
    uint16_t pulse_default_ns;
    float    dynamic_range_db;
    uint32_t flags;
} rv_builder_inst;

typedef struct rv_builder_calib {
    uint32_t calib_id;
    uint32_t inst_id;
    uint64_t calibrated_unix;
    float    refractive_index;
    float    backscatter_coef;
    float    splice_loss_db;
    uint32_t flags;
} rv_builder_calib;

typedef struct rv_builder_window {
    uint32_t window_id;
    uint32_t route_id;
    uint32_t inst_id;
    uint32_t calib_id;
    uint32_t wave_block_id;
    float    pulse_ns;
    float    range_km;
    float    resolution_m;
    uint64_t acquired_unix;
} rv_builder_window;

typedef struct rv_builder_wave {
    uint32_t block_id;
    uint32_t window_id;
    uint32_t sample_count;
    float    start_m;
    float    step_m;
    int16_t *samples; /* owned by builder arena or heap */
    int      samples_owned;
    uint16_t flags;
} rv_builder_wave;

typedef struct rv_builder_marker {
    uint32_t marker_id;
    uint32_t window_id;
    uint32_t label_name_id;
    float    distance_m;
    float    loss_db;
    float    reflectance_db;
    uint16_t kind;
    uint16_t severity;
    uint32_t flags;
} rv_builder_marker;

typedef struct rv_builder_link {
    uint32_t link_id;
    uint32_t from_kind;
    uint32_t from_id;
    uint32_t to_kind;
    uint32_t to_id;
    uint32_t flags;
} rv_builder_link;

typedef struct rv_builder_note {
    uint32_t note_id;
    uint32_t target_kind;
    uint32_t target_id;
    uint32_t text_name_id;
    uint32_t flags;
} rv_builder_note;

typedef struct rv_builder {
    rv_arena arena;
    rv_builder_name   *names;
    uint32_t name_count, name_cap;
    rv_builder_route  *routes;
    uint32_t route_count, route_cap;
    rv_builder_inst   *insts;
    uint32_t inst_count, inst_cap;
    rv_builder_calib  *calibs;
    uint32_t calib_count, calib_cap;
    rv_builder_window *windows;
    uint32_t window_count, window_cap;
    rv_builder_wave   *waves;
    uint32_t wave_count, wave_cap;
    rv_builder_marker *markers;
    uint32_t marker_count, marker_cap;
    rv_builder_link   *links;
    uint32_t link_count, link_cap;
    rv_builder_note   *notes;
    uint32_t note_count, note_cap;
    uint32_t next_name_id;
    uint16_t ver_major;
    uint16_t ver_minor;
    uint32_t feature_bits;
    uint64_t created_unix;
    int      include_summ;
    int      include_notes;
} rv_builder;

int  rv_builder_init(rv_builder *b);
void rv_builder_destroy(rv_builder *b);
void rv_builder_set_version(rv_builder *b, uint16_t major, uint16_t minor);
void rv_builder_set_created(rv_builder *b, uint64_t unix_ts);

int rv_builder_add_name(rv_builder *b, const char *str, uint32_t *out_id);
int rv_builder_add_route(rv_builder *b, const rv_builder_route *r);
int rv_builder_add_inst(rv_builder *b, const rv_builder_inst *i);
int rv_builder_add_calib(rv_builder *b, const rv_builder_calib *c);
int rv_builder_add_window(rv_builder *b, const rv_builder_window *w);
int rv_builder_add_wave(rv_builder *b, const rv_builder_wave *w);
int rv_builder_add_marker(rv_builder *b, const rv_builder_marker *m);
int rv_builder_add_link(rv_builder *b, const rv_builder_link *l);
int rv_builder_add_note(rv_builder *b, const rv_builder_note *n);

int rv_builder_add_wave_copy(rv_builder *b, uint32_t block_id,
                             uint32_t window_id, const int16_t *samples,
                             uint32_t count, float start_m, float step_m);

int rv_builder_serialize(rv_builder *b, rv_buf *out);
int rv_builder_write_file(rv_builder *b, const char *path);

/* Convenience: build a minimal valid multi-section package. */
int rv_builder_demo_span(rv_builder *b, const char *site, const char *far,
                         float length_km, uint32_t sample_count);

#endif /* RV_BUILDER_H */
