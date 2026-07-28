/**
 * rayvault.h — public C API for RayVault optical fiber monitoring packages.
 *
 * RayVault reads, indexes, streams, validates, and exports RVP datasets
 * produced by OTDR acquisition systems and fiber plant monitoring tools.
 */
#ifndef RAYVAULT_H
#define RAYVAULT_H

#include "rayvault_error.h"
#include "rayvault_types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rv_package rv_package;
typedef struct rv_session rv_session;
typedef struct rv_stream  rv_stream;
typedef struct rv_cursor  rv_cursor;
typedef struct rv_checkpoint rv_checkpoint;

typedef struct rv_config {
    size_t   arena_initial_bytes;
    size_t   arena_max_bytes;
    size_t   cache_capacity;
    size_t   stream_prefetch;
    uint32_t options; /* bitmask of rv_open_flags defaults */
    int      log_level; /* 0=off .. 4=trace */
} rv_config;

typedef void (*rv_notify_fn)(void *userdata, int event, const char *msg);

void rv_config_init(rv_config *cfg);

/* --- library --- */
const char *rv_version_string(void);
rv_version  rv_library_version(void);

/* --- package open / close --- */
rv_status rv_package_open_file(rv_package **out, const char *path,
                               uint32_t flags, const rv_config *cfg);
rv_status rv_package_open_memory(rv_package **out, const uint8_t *data,
                                 size_t size, uint32_t flags,
                                 const rv_config *cfg);
rv_status rv_package_reopen(rv_package *pkg);
rv_status rv_package_reset(rv_package *pkg);
void      rv_package_close(rv_package *pkg);

rv_status rv_package_get_info(const rv_package *pkg, rv_package_info *info);
rv_status rv_package_validate(rv_package *pkg, uint32_t strictness);

/* --- session (binds index, cache, derived state to a package) --- */
rv_status rv_session_create(rv_session **out, rv_package *pkg);
rv_status rv_session_rebuild_index(rv_session *sess);
rv_status rv_session_warm_cache(rv_session *sess);
void      rv_session_destroy(rv_session *sess);

rv_status rv_session_stats(const rv_session *sess, rv_stats_snapshot *stats);
rv_status rv_session_set_notify(rv_session *sess, rv_notify_fn fn, void *ud);

/* --- dictionary / name lookup --- */
rv_status rv_name_lookup(const rv_session *sess, uint32_t name_id,
                         const char **out_str, size_t *out_len);
rv_status rv_name_find(const rv_session *sess, const char *str,
                       uint32_t *out_id);

/* --- route / instrument / calibration queries --- */
rv_status rv_route_count(const rv_session *sess, uint32_t *out);
rv_status rv_route_at(const rv_session *sess, uint32_t index,
                      uint32_t *route_id, uint32_t *site_name_id,
                      float *length_km, uint32_t *fiber_count);
rv_status rv_instrument_count(const rv_session *sess, uint32_t *out);
rv_status rv_instrument_at(const rv_session *sess, uint32_t index,
                           uint32_t *inst_id, uint32_t *model_name_id,
                           uint16_t *wavelength_nm);

/* --- window / marker / wave access --- */
rv_status rv_window_count(const rv_session *sess, uint32_t *out);
rv_status rv_window_find(const rv_session *sess, const rv_window_key *key,
                         uint32_t *out_index);
rv_status rv_window_get(const rv_session *sess, uint32_t index,
                        rv_window_key *key, float *pulse_ns,
                        uint32_t *wave_block_id);

rv_status rv_marker_query(const rv_session *sess, const rv_query_filter *flt,
                          rv_marker_info *out, size_t cap, size_t *written);
rv_status rv_wave_get(const rv_session *sess, uint32_t wave_block_id,
                      rv_wave_slice *out);

/* --- streaming / cursors --- */
rv_status rv_stream_open(rv_stream **out, rv_session *sess, uint32_t route_id);
rv_status rv_stream_next_window(rv_stream *st, uint32_t *window_index);
rv_status rv_stream_next_wave(rv_stream *st, rv_wave_slice *slice);
void      rv_stream_close(rv_stream *st);

rv_status rv_cursor_open(rv_cursor **out, rv_session *sess);
rv_status rv_cursor_seek_window(rv_cursor *cur, uint32_t window_id);
rv_status rv_cursor_read_wave(rv_cursor *cur, rv_wave_slice *slice);
rv_status rv_cursor_advance(rv_cursor *cur);
void      rv_cursor_close(rv_cursor *cur);

/* --- checkpoint / export / recovery --- */
rv_status rv_checkpoint_create(rv_checkpoint **out, rv_session *sess);
rv_status rv_checkpoint_restore(rv_session *sess, rv_checkpoint *cp);
void      rv_checkpoint_destroy(rv_checkpoint *cp);

rv_status rv_export_buffer(rv_session *sess, uint32_t flags,
                           uint8_t **out_buf, size_t *out_size);
void      rv_export_free(uint8_t *buf);

rv_status rv_package_repair(rv_package *pkg, uint32_t repair_flags);
rv_status rv_package_migrate_legacy(rv_package *pkg);

/* --- diagnostics --- */
const char *rv_package_last_error(const rv_package *pkg);
int         rv_package_diag_count(const rv_package *pkg);
rv_status   rv_package_diag_at(const rv_package *pkg, int index,
                               int *code, const char **message);

#ifdef __cplusplus
}
#endif

#endif /* RAYVAULT_H */
