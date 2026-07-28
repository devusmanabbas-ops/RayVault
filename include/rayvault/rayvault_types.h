/**
 * rayvault_types.h — shared public types for fiber monitoring packages.
 */
#ifndef RAYVAULT_TYPES_H
#define RAYVAULT_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RV_MAGIC0 'R'
#define RV_MAGIC1 'V'
#define RV_MAGIC2 'P'
#define RV_MAGIC3 'K'

#define RV_FORMAT_VERSION_MAJOR 2
#define RV_FORMAT_VERSION_MINOR 3
#define RV_FORMAT_VERSION_LEGACY_MAJOR 1

/* Section fourcc tags (little-endian on disk as raw bytes). */
#define RV_TAG_SNAM 0x4D414E53u /* SNAM — site / span name pool        */
#define RV_TAG_ROUT 0x54554F52u /* ROUT — fiber route table            */
#define RV_TAG_INST 0x54534E49u /* INST — instrument / probe defs      */
#define RV_TAG_CLBR 0x52424C43u /* CLBR — calibration records          */
#define RV_TAG_WIND 0x444E4957u /* WIND — acquisition windows          */
#define RV_TAG_WAVE 0x45564157u /* WAVE — waveform sample payloads     */
#define RV_TAG_MARK 0x4B52414Du /* MARK — event / marker annotations   */
#define RV_TAG_LINK 0x4B4E494Cu /* LINK — cross-reference linkages     */
#define RV_TAG_NOTE 0x45544F4Eu /* NOTE — optional extension notes     */
#define RV_TAG_SUMM 0x4D4D5553u /* SUMM — footer summary / checksums   */

typedef enum rv_open_flags {
    RV_OPEN_DEFAULT       = 0,
    RV_OPEN_READONLY      = 1u << 0,
    RV_OPEN_STRICT        = 1u << 1,
    RV_OPEN_ALLOW_LEGACY  = 1u << 2,
    RV_OPEN_BUILD_INDEX   = 1u << 3,
    RV_OPEN_WARM_CACHE    = 1u << 4,
    RV_OPEN_SKIP_SUMM     = 1u << 5,
    RV_OPEN_REPAIR_HINTS  = 1u << 6
} rv_open_flags;

typedef enum rv_export_flags {
    RV_EXPORT_DEFAULT      = 0,
    RV_EXPORT_INCLUDE_WAVE = 1u << 0,
    RV_EXPORT_COMPACT      = 1u << 1,
    RV_EXPORT_WITH_SUMMARY = 1u << 2,
    RV_EXPORT_LEGACY_V1    = 1u << 3
} rv_export_flags;

typedef struct rv_version {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
} rv_version;

typedef struct rv_span_id {
    uint32_t route_id;
    uint32_t site_name_id;
} rv_span_id;

typedef struct rv_window_key {
    uint32_t window_id;
    uint32_t route_id;
    uint32_t instrument_id;
} rv_window_key;

typedef struct rv_marker_info {
    uint32_t marker_id;
    uint32_t window_id;
    float    distance_m;
    uint16_t kind;
    uint16_t severity;
    uint32_t label_name_id;
} rv_marker_info;

typedef struct rv_wave_slice {
    const int16_t *samples;
    size_t         count;
    float          start_m;
    float          step_m;
    uint32_t       window_id;
} rv_wave_slice;

typedef struct rv_package_info {
    rv_version format;
    uint32_t   flags;
    uint32_t   section_count;
    uint64_t   created_unix;
    uint64_t   package_bytes;
    uint32_t   route_count;
    uint32_t   window_count;
    uint32_t   marker_count;
    uint32_t   wave_block_count;
} rv_package_info;

typedef struct rv_query_filter {
    uint32_t route_id;       /* 0 = any */
    uint32_t instrument_id;  /* 0 = any */
    uint16_t marker_kind;    /* 0 = any */
    uint16_t min_severity;
    float    min_distance_m;
    float    max_distance_m;
} rv_query_filter;

typedef struct rv_stats_snapshot {
    uint32_t routes;
    uint32_t instruments;
    uint32_t calibrations;
    uint32_t windows;
    uint32_t wave_blocks;
    uint32_t markers;
    uint32_t linkages;
    uint32_t name_entries;
    uint64_t total_samples;
    double   mean_span_km;
    double   max_loss_db;
} rv_stats_snapshot;

#ifdef __cplusplus
}
#endif

#endif /* RAYVAULT_TYPES_H */
