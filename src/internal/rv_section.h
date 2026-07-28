#ifndef RV_SECTION_H
#define RV_SECTION_H

#include "rv_arena.h"
#include "rv_format.h"
#include "rv_reader.h"

/*
 * Section payload decoders. Each decoder allocates record tables from the
 * supplied arena and may retain pointers into the package backing store
 * for large sample payloads (WAVE).
 */

typedef struct rv_snam_table {
    rv_name_entry *entries;
    uint32_t       count;
    uint32_t      *bucket; /* hash -> index+1, 0 empty */
    uint32_t       bucket_mask;
} rv_snam_table;

typedef struct rv_rout_table {
    rv_route_rec *items;
    uint32_t      count;
} rv_rout_table;

typedef struct rv_inst_table {
    rv_inst_rec *items;
    uint32_t     count;
} rv_inst_table;

typedef struct rv_clbr_table {
    rv_calib_rec *items;
    uint32_t      count;
} rv_clbr_table;

typedef struct rv_wind_table {
    rv_window_rec *items;
    uint32_t       count;
} rv_wind_table;

typedef struct rv_wave_table {
    rv_wave_block *items;
    uint32_t       count;
} rv_wave_table;

typedef struct rv_mark_table {
    rv_marker_rec *items;
    uint32_t       count;
} rv_mark_table;

typedef struct rv_link_table {
    rv_link_rec *items;
    uint32_t     count;
} rv_link_table;

typedef struct rv_note_table {
    rv_note_rec *items;
    uint32_t     count;
} rv_note_table;

int rv_decode_snam(rv_arena *a, rv_slice payload, rv_snam_table *out);
int rv_decode_rout(rv_arena *a, rv_slice payload, rv_rout_table *out);
int rv_decode_inst(rv_arena *a, rv_slice payload, rv_inst_table *out);
int rv_decode_clbr(rv_arena *a, rv_slice payload, rv_clbr_table *out);
int rv_decode_wind(rv_arena *a, rv_slice payload, rv_wind_table *out);
int rv_decode_wave(rv_arena *a, rv_slice payload, rv_wave_table *out);
int rv_decode_mark(rv_arena *a, rv_slice payload, rv_mark_table *out);
int rv_decode_link(rv_arena *a, rv_slice payload, rv_link_table *out);
int rv_decode_note(rv_arena *a, rv_slice payload, rv_note_table *out);
int rv_decode_summ(rv_slice payload, rv_summ_rec *out);

const char *rv_tag_name(uint32_t tag);

#endif /* RV_SECTION_H */
