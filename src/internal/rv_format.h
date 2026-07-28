#ifndef RV_FORMAT_H
#define RV_FORMAT_H

#include "rayvault/rayvault_types.h"
#include "rv_buf.h"

#include <stddef.h>
#include <stdint.h>

/*
 * On-disk RVP layout (version 2.x):
 *
 *   [file header 64 bytes]
 *   [section directory: N * 24 bytes]
 *   [section payloads ...]
 *   [optional trailing SUMM mirrored in directory]
 *
 * Directory entries reference absolute file offsets. Cross-section
 * integrity depends on SNAM being present before ROUT/INST/MARK
 * consumers resolve name ids, and WAVE blocks being addressable from
 * WIND records via wave_block_id.
 */

#define RV_FILE_HEADER_SIZE 64
#define RV_DIR_ENTRY_SIZE   24
#define RV_MAX_SECTIONS     64

typedef struct rv_file_header {
    uint8_t  magic[4];
    uint16_t ver_major;
    uint16_t ver_minor;
    uint32_t flags;
    uint32_t header_crc;
    uint64_t created_unix;
    uint32_t section_count;
    uint32_t dir_offset;
    uint64_t total_size;
    uint32_t feature_bits;
    uint32_t reserved0;
    uint64_t reserved1;
} rv_file_header;

typedef struct rv_dir_entry {
    uint32_t tag;
    uint32_t flags;
    uint64_t offset;
    uint32_t length;
    uint32_t crc32;
} rv_dir_entry;

/* --- decoded in-memory records --- */

typedef struct rv_name_entry {
    uint32_t id;
    uint32_t hash;
    const char *str;   /* arena-backed */
    uint16_t len;
    uint16_t reserved;
} rv_name_entry;

typedef struct rv_route_rec {
    uint32_t route_id;
    uint32_t site_name_id;
    uint32_t far_name_id;
    float    length_km;
    uint32_t fiber_count;
    uint32_t flags;
} rv_route_rec;

typedef struct rv_inst_rec {
    uint32_t inst_id;
    uint32_t model_name_id;
    uint32_t serial_name_id;
    uint16_t wavelength_nm;
    uint16_t pulse_default_ns;
    float    dynamic_range_db;
    uint32_t flags;
} rv_inst_rec;

typedef struct rv_calib_rec {
    uint32_t calib_id;
    uint32_t inst_id;
    uint64_t calibrated_unix;
    float    refractive_index;
    float    backscatter_coef;
    float    splice_loss_db;
    uint32_t flags;
} rv_calib_rec;

typedef struct rv_window_rec {
    uint32_t window_id;
    uint32_t route_id;
    uint32_t inst_id;
    uint32_t calib_id;
    uint32_t wave_block_id;
    float    pulse_ns;
    float    range_km;
    float    resolution_m;
    uint64_t acquired_unix;
    uint32_t flags;
} rv_window_rec;

typedef struct rv_wave_block {
    uint32_t block_id;
    uint32_t window_id;
    uint32_t sample_count;
    float    start_m;
    float    step_m;
    const int16_t *samples; /* points into package backing or arena copy */
    uint32_t flags;
    uint32_t payload_crc;
} rv_wave_block;

typedef struct rv_marker_rec {
    uint32_t marker_id;
    uint32_t window_id;
    uint32_t label_name_id;
    float    distance_m;
    float    loss_db;
    float    reflectance_db;
    uint16_t kind;
    uint16_t severity;
    uint32_t flags;
} rv_marker_rec;

typedef struct rv_link_rec {
    uint32_t link_id;
    uint32_t from_kind; /* 1=route 2=window 3=marker 4=wave */
    uint32_t from_id;
    uint32_t to_kind;
    uint32_t to_id;
    uint32_t flags;
} rv_link_rec;

typedef struct rv_note_rec {
    uint32_t note_id;
    uint32_t target_kind;
    uint32_t target_id;
    uint32_t text_name_id;
    uint32_t flags;
} rv_note_rec;

typedef struct rv_summ_rec {
    uint32_t route_count;
    uint32_t window_count;
    uint32_t marker_count;
    uint32_t wave_count;
    uint32_t name_count;
    uint32_t package_crc;
    uint64_t payload_bytes;
    uint32_t flags;
} rv_summ_rec;

int rv_parse_file_header(const uint8_t *data, size_t size, rv_file_header *out);
int rv_parse_dir_entry(const uint8_t *data, size_t size, rv_dir_entry *out);
uint32_t rv_header_crc_compute(const rv_file_header *h);

#endif /* RV_FORMAT_H */
