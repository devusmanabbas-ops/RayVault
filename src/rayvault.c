#include "rv_package.h"
#include "rv_dict.h"
#include "rv_export.h"
#include "rv_legacy.h"
#include "rv_log.h"
#include "rv_normalize.h"
#include "rv_recover.h"
#include "rv_validate.h"
#include "rv_xref.h"

#include <stdlib.h>
#include <string.h>

static const char *RV_VERSION = "2.3.1";

const char *rv_version_string(void)
{
    return RV_VERSION;
}

rv_version rv_library_version(void)
{
    rv_version v;
    v.major = 2;
    v.minor = 3;
    v.patch = 1;
    v.reserved = 0;
    return v;
}

const char *rv_status_string(rv_status st)
{
    switch (st) {
    case RV_OK: return "ok";
    case RV_ERR_INVALID_ARG: return "invalid argument";
    case RV_ERR_NOMEM: return "out of memory";
    case RV_ERR_IO: return "io error";
    case RV_ERR_FORMAT: return "format error";
    case RV_ERR_VERSION: return "version error";
    case RV_ERR_CHECKSUM: return "checksum error";
    case RV_ERR_TRUNCATED: return "truncated";
    case RV_ERR_SECTION: return "section error";
    case RV_ERR_OFFSET: return "offset error";
    case RV_ERR_CROSSREF: return "cross-reference error";
    case RV_ERR_STATE: return "bad state";
    case RV_ERR_NOT_FOUND: return "not found";
    case RV_ERR_OVERFLOW: return "overflow";
    case RV_ERR_UNSUPPORTED: return "unsupported";
    case RV_ERR_LEGACY: return "legacy error";
    case RV_ERR_REPAIR: return "repair error";
    case RV_ERR_CACHE: return "cache error";
    case RV_ERR_CURSOR: return "cursor error";
    case RV_ERR_EXPORT: return "export error";
    case RV_ERR_CHECKPOINT: return "checkpoint error";
    case RV_ERR_CONFIG: return "config error";
    case RV_ERR_INTERNAL: return "internal error";
    case RV_ERR_AGAIN: return "try again";
    case RV_ERR_CLOSED: return "closed";
    case RV_ERR_BUSY: return "busy";
    default: return "unknown";
    }
}

const char *rv_status_detail(rv_status st)
{
    return rv_status_string(st);
}

static rv_package *rv_package_alloc(const rv_config *cfg, uint32_t flags)
{
    rv_package *pkg = (rv_package *)calloc(1, sizeof(*pkg));
    if (!pkg)
        return NULL;
    if (cfg)
        pkg->cfg = *cfg;
    else
        rv_config_init(&pkg->cfg);
    rv_config_sanitize(&pkg->cfg);
    pkg->flags = flags | pkg->cfg.options;
    rv_log_set_level(pkg->cfg.log_level);
    if (rv_arena_init(&pkg->arena, rv_config_arena_initial(&pkg->cfg),
                      rv_config_arena_max(&pkg->cfg)) != 0) {
        free(pkg);
        return NULL;
    }
    rv_diag_clear(&pkg->diag);
    return pkg;
}

rv_status rv_package_finish_open(rv_package *pkg)
{
    rv_normalize_stats ns;
    if (!pkg)
        return RV_ERR_INVALID_ARG;
    rv_parser_init(&pkg->parser, &pkg->reader, &pkg->arena, &pkg->diag,
                   pkg->flags);
    if (rv_parser_run(&pkg->parser) != 0)
        return RV_ERR_FORMAT;
    (void)rv_normalize_parsed(&pkg->parser.result, &ns);
    pkg->legacy = rv_legacy_is_v1(&pkg->reader);
    pkg->opened = 1;
    return RV_OK;
}

rv_status rv_package_open_file(rv_package **out, const char *path,
                               uint32_t flags, const rv_config *cfg)
{
    rv_package *pkg;
    if (!out || !path)
        return RV_ERR_INVALID_ARG;
    pkg = rv_package_alloc(cfg, flags);
    if (!pkg)
        return RV_ERR_NOMEM;
    if (rv_reader_open_file(&pkg->reader, path, pkg->flags) != 0) {
        rv_status st = RV_ERR_IO;
        if (pkg->diag.count)
            st = pkg->diag.entries[0].code;
        rv_arena_destroy(&pkg->arena);
        free(pkg);
        return st;
    }
    {
        rv_status st = rv_package_finish_open(pkg);
        if (st != RV_OK) {
            rv_reader_close(&pkg->reader);
            rv_arena_destroy(&pkg->arena);
            free(pkg);
            return st;
        }
    }
    *out = pkg;
    return RV_OK;
}

rv_status rv_package_open_memory(rv_package **out, const uint8_t *data,
                                 size_t size, uint32_t flags,
                                 const rv_config *cfg)
{
    rv_package *pkg;
    if (!out || !data)
        return RV_ERR_INVALID_ARG;
    pkg = rv_package_alloc(cfg, flags);
    if (!pkg)
        return RV_ERR_NOMEM;
    if (rv_reader_open_memory(&pkg->reader, data, size, pkg->flags, 1) != 0) {
        rv_arena_destroy(&pkg->arena);
        free(pkg);
        return RV_ERR_FORMAT;
    }
    {
        rv_status st = rv_package_finish_open(pkg);
        if (st != RV_OK) {
            rv_reader_close(&pkg->reader);
            rv_arena_destroy(&pkg->arena);
            free(pkg);
            return st;
        }
    }
    *out = pkg;
    return RV_OK;
}

rv_status rv_package_reopen(rv_package *pkg)
{
    if (!pkg || !pkg->opened)
        return RV_ERR_STATE;
    if (!pkg->reader.from_file)
        return RV_ERR_UNSUPPORTED;

    rv_arena_reset(&pkg->arena);
    if (rv_reader_reload(&pkg->reader) != 0)
        return RV_ERR_IO;
    return rv_package_finish_open(pkg);
}

rv_status rv_package_reset(rv_package *pkg)
{
    if (!pkg || !pkg->opened)
        return RV_ERR_STATE;
    rv_parser_reset(&pkg->parser);
    return rv_package_finish_open(pkg);
}

/*
 * Close clears the package struct after releasing nested resources.
 * Historical cleanup split header teardown and nested free helpers;
 * rv_package_close zeroes early so a secondary nested release sees
 * empty fields (no-op). Session destroy is responsible for cache/index.
 */
void rv_package_close(rv_package *pkg)
{
    if (!pkg)
        return;
    pkg->opened = 0;
    rv_reader_close(&pkg->reader);
    rv_arena_destroy(&pkg->arena);
    memset(pkg, 0, sizeof(*pkg));
    free(pkg);
}

rv_status rv_package_get_info(const rv_package *pkg, rv_package_info *info)
{
    const rv_parsed *p;
    if (!pkg || !info || !pkg->opened)
        return RV_ERR_INVALID_ARG;
    memset(info, 0, sizeof(*info));
    p = &pkg->parser.result;
    info->format.major = pkg->reader.header.ver_major;
    info->format.minor = pkg->reader.header.ver_minor;
    info->flags = pkg->reader.header.flags;
    info->section_count = pkg->reader.dir_count;
    info->created_unix = pkg->reader.header.created_unix;
    info->package_bytes = pkg->reader.backing.size;
    info->route_count = p->rout.count;
    info->window_count = p->wind.count;
    info->marker_count = p->mark.count;
    info->wave_block_count = p->wave.count;
    return RV_OK;
}

rv_status rv_package_validate(rv_package *pkg, uint32_t strictness)
{
    rv_validate_options o;
    rv_index tmp;
    int rc;
    if (!pkg || !pkg->opened)
        return RV_ERR_STATE;
    rv_validate_options_default(&o);
    if (strictness == 0)
        o.check_crc = 0;
    memset(&tmp, 0, sizeof(tmp));
    if (rv_index_build(&tmp, &pkg->arena, &pkg->parser.result) != 0)
        return RV_ERR_NOMEM;
    rc = rv_validate_package(&pkg->reader, &pkg->parser.result, &tmp, &o,
                             &pkg->diag);
    return rc == 0 ? RV_OK : RV_ERR_FORMAT;
}

const char *rv_package_last_error(const rv_package *pkg)
{
    return pkg ? pkg->diag.last : "";
}

int rv_package_diag_count(const rv_package *pkg)
{
    return pkg ? rv_diag_count(&pkg->diag) : 0;
}

rv_status rv_package_diag_at(const rv_package *pkg, int index,
                             int *code, const char **message)
{
    if (!pkg)
        return RV_ERR_INVALID_ARG;
    return rv_diag_at(&pkg->diag, index, code, message);
}

/* ---------------- session ---------------- */

rv_status rv_session_ensure_index(rv_session *sess)
{
    if (!sess || !sess->pkg)
        return RV_ERR_STATE;
    if (sess->index_ready)
        return RV_OK;
    if (rv_index_build(&sess->index, &sess->pkg->arena,
                       &sess->pkg->parser.result) != 0)
        return RV_ERR_NOMEM;
    sess->index_ready = 1;
    rv_notify_emit(&sess->notify, RV_EVENT_INDEXED, "index ready");
    return RV_OK;
}

rv_status rv_session_create(rv_session **out, rv_package *pkg)
{
    rv_session *sess;
    if (!out || !pkg || !pkg->opened)
        return RV_ERR_INVALID_ARG;
    sess = (rv_session *)calloc(1, sizeof(*sess));
    if (!sess)
        return RV_ERR_NOMEM;
    sess->pkg = pkg;
    if (rv_cache_init(&sess->cache, rv_config_cache_capacity(&pkg->cfg)) != 0) {
        free(sess);
        return RV_ERR_NOMEM;
    }
    if (pkg->flags & RV_OPEN_BUILD_INDEX) {
        rv_status st = rv_session_ensure_index(sess);
        if (st != RV_OK) {
            rv_cache_destroy(&sess->cache);
            free(sess);
            return st;
        }
    }
    if (pkg->flags & RV_OPEN_WARM_CACHE)
        (void)rv_session_warm_cache(sess);
    *out = sess;
    return RV_OK;
}

rv_status rv_session_rebuild_index(rv_session *sess)
{
    if (!sess || !sess->pkg)
        return RV_ERR_STATE;
    /*
     * Rebuild replaces index tables in the same arena without resetting
     * the arena epoch. Derived summary is left intact so export can still
     * consult the previous snapshot until explicitly refreshed.
     */
    sess->index_ready = 0;
    rv_index_clear(&sess->index);
    {
        rv_status st = rv_session_ensure_index(sess);
        if (st != RV_OK)
            return st;
    }
    /* Intentionally do not call rv_derived_summary_invalidate here. */
    return RV_OK;
}

rv_status rv_session_warm_cache(rv_session *sess)
{
    uint32_t i;
    rv_status st;
    if (!sess)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index(sess);
    if (st != RV_OK)
        return st;
    for (i = 0; i < sess->pkg->parser.result.wave.count; i++) {
        const rv_wave_block *b = &sess->pkg->parser.result.wave.items[i];
        (void)rv_cache_put_from_block(&sess->cache, b);
        if (sess->cache.count >= sess->cache.capacity)
            break;
    }
    if (!sess->derived.valid)
        (void)rv_derived_summary_build(&sess->derived, &sess->pkg->arena,
                                       &sess->pkg->parser.result,
                                       &sess->index);
    rv_notify_emit(&sess->notify, RV_EVENT_CACHE_WARM, "cache warmed");
    return RV_OK;
}

void rv_session_destroy(rv_session *sess)
{
    if (!sess)
        return;
    rv_cache_destroy(&sess->cache);
    rv_derived_summary_invalidate(&sess->derived);
    memset(sess, 0, sizeof(*sess));
    free(sess);
}

rv_status rv_session_stats(const rv_session *sess, rv_stats_snapshot *stats)
{
    if (!sess || !stats)
        return RV_ERR_INVALID_ARG;
    if (sess->derived.valid) {
        *stats = sess->derived.snap;
        return RV_OK;
    }
    return rv_stats_compute(&sess->pkg->parser.result, &sess->index, stats) == 0
               ? RV_OK
               : RV_ERR_INTERNAL;
}

rv_status rv_session_set_notify(rv_session *sess, rv_notify_fn fn, void *ud)
{
    if (!sess)
        return RV_ERR_INVALID_ARG;
    sess->notify.fn = fn;
    sess->notify.userdata = ud;
    return RV_OK;
}

rv_status rv_name_lookup(const rv_session *sess, uint32_t name_id,
                         const char **out_str, size_t *out_len)
{
    if (!sess)
        return RV_ERR_INVALID_ARG;
    if (rv_dict_resolve(&sess->pkg->parser.result.snam, name_id, out_str,
                        out_len) != 0)
        return RV_ERR_NOT_FOUND;
    return RV_OK;
}

rv_status rv_name_find(const rv_session *sess, const char *str,
                       uint32_t *out_id)
{
    const rv_name_entry *e;
    if (!sess || !str || !out_id)
        return RV_ERR_INVALID_ARG;
    e = rv_dict_by_str(&sess->pkg->parser.result.snam, str, strlen(str));
    if (!e)
        return RV_ERR_NOT_FOUND;
    *out_id = e->id;
    return RV_OK;
}

rv_status rv_route_count(const rv_session *sess, uint32_t *out)
{
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    *out = sess->pkg->parser.result.rout.count;
    return RV_OK;
}

rv_status rv_route_at(const rv_session *sess, uint32_t index,
                      uint32_t *route_id, uint32_t *site_name_id,
                      float *length_km, uint32_t *fiber_count)
{
    const rv_route_rec *r;
    if (!sess || index >= sess->pkg->parser.result.rout.count)
        return RV_ERR_INVALID_ARG;
    r = &sess->pkg->parser.result.rout.items[index];
    if (route_id)
        *route_id = r->route_id;
    if (site_name_id)
        *site_name_id = r->site_name_id;
    if (length_km)
        *length_km = r->length_km;
    if (fiber_count)
        *fiber_count = r->fiber_count;
    return RV_OK;
}

rv_status rv_instrument_count(const rv_session *sess, uint32_t *out)
{
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    *out = sess->pkg->parser.result.inst.count;
    return RV_OK;
}

rv_status rv_instrument_at(const rv_session *sess, uint32_t index,
                           uint32_t *inst_id, uint32_t *model_name_id,
                           uint16_t *wavelength_nm)
{
    const rv_inst_rec *r;
    if (!sess || index >= sess->pkg->parser.result.inst.count)
        return RV_ERR_INVALID_ARG;
    r = &sess->pkg->parser.result.inst.items[index];
    if (inst_id)
        *inst_id = r->inst_id;
    if (model_name_id)
        *model_name_id = r->model_name_id;
    if (wavelength_nm)
        *wavelength_nm = r->wavelength_nm;
    return RV_OK;
}

rv_status rv_window_count(const rv_session *sess, uint32_t *out)
{
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    *out = sess->pkg->parser.result.wind.count;
    return RV_OK;
}

rv_status rv_window_find(const rv_session *sess, const rv_window_key *key,
                         uint32_t *out_index)
{
    uint32_t i;
    if (!sess || !key || !out_index)
        return RV_ERR_INVALID_ARG;
    for (i = 0; i < sess->pkg->parser.result.wind.count; i++) {
        const rv_window_rec *w = &sess->pkg->parser.result.wind.items[i];
        if (key->window_id && w->window_id != key->window_id)
            continue;
        if (key->route_id && w->route_id != key->route_id)
            continue;
        if (key->instrument_id && w->inst_id != key->instrument_id)
            continue;
        *out_index = i;
        return RV_OK;
    }
    return RV_ERR_NOT_FOUND;
}

rv_status rv_window_get(const rv_session *sess, uint32_t index,
                        rv_window_key *key, float *pulse_ns,
                        uint32_t *wave_block_id)
{
    const rv_window_rec *w;
    if (!sess || index >= sess->pkg->parser.result.wind.count)
        return RV_ERR_INVALID_ARG;
    w = &sess->pkg->parser.result.wind.items[index];
    if (key) {
        key->window_id = w->window_id;
        key->route_id = w->route_id;
        key->instrument_id = w->inst_id;
    }
    if (pulse_ns)
        *pulse_ns = w->pulse_ns;
    if (wave_block_id)
        *wave_block_id = w->wave_block_id;
    return RV_OK;
}

rv_status rv_marker_query(const rv_session *sess, const rv_query_filter *flt,
                          rv_marker_info *out, size_t cap, size_t *written)
{
    rv_status st;
    if (!sess)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index((rv_session *)sess);
    if (st != RV_OK)
        return st;
    if (rv_index_query_markers(&sess->index, flt, out, cap, written) != 0)
        return RV_ERR_INTERNAL;
    return RV_OK;
}

rv_status rv_wave_get(const rv_session *sess, uint32_t wave_block_id,
                      rv_wave_slice *out)
{
    uint32_t pos;
    rv_cache_entry *e;
    const rv_wave_block *b;
    rv_status st;
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index((rv_session *)sess);
    if (st != RV_OK)
        return st;
    if (rv_index_find_wave(&sess->index, wave_block_id, &pos) != 0)
        return RV_ERR_NOT_FOUND;
    b = sess->index.waves[pos].block;
    if (rv_cache_get((rv_wave_cache *)&sess->cache, wave_block_id, &e) != 0) {
        if (rv_cache_put_from_block((rv_wave_cache *)&sess->cache, b) != 0)
            return RV_ERR_CACHE;
        if (rv_cache_get((rv_wave_cache *)&sess->cache, wave_block_id, &e) != 0)
            return RV_ERR_CACHE;
    }
    out->samples = e->samples;
    out->count = e->sample_count;
    out->start_m = e->start_m;
    out->step_m = e->step_m;
    out->window_id = e->window_id;
    return RV_OK;
}

/* ---------------- stream / cursor ---------------- */

rv_status rv_stream_open(rv_stream **out, rv_session *sess, uint32_t route_id)
{
    rv_stream *st;
    rv_status rc;
    if (!out || !sess)
        return RV_ERR_INVALID_ARG;
    rc = rv_session_ensure_index(sess);
    if (rc != RV_OK)
        return rc;
    st = (rv_stream *)calloc(1, sizeof(*st));
    if (!st)
        return RV_ERR_NOMEM;
    st->sess = sess;
    if (rv_stream_impl_open(&st->impl, &sess->pkg->parser.result, &sess->index,
                            &sess->cache, &sess->pkg->arena, route_id) != 0) {
        free(st);
        return RV_ERR_INTERNAL;
    }
    *out = st;
    return RV_OK;
}

rv_status rv_stream_next_window(rv_stream *st, uint32_t *window_index)
{
    if (!st)
        return RV_ERR_INVALID_ARG;
    return rv_stream_impl_next_window(&st->impl, window_index) == 0
               ? RV_OK
               : RV_ERR_NOT_FOUND;
}

rv_status rv_stream_next_wave(rv_stream *st, rv_wave_slice *slice)
{
    if (!st)
        return RV_ERR_INVALID_ARG;
    return rv_stream_impl_next_wave(&st->impl, slice) == 0 ? RV_OK
                                                           : RV_ERR_CURSOR;
}

void rv_stream_close(rv_stream *st)
{
    if (!st)
        return;
    rv_stream_impl_close(&st->impl);
    free(st);
}

rv_status rv_cursor_open(rv_cursor **out, rv_session *sess)
{
    rv_cursor *cur;
    rv_status rc;
    if (!out || !sess)
        return RV_ERR_INVALID_ARG;
    rc = rv_session_ensure_index(sess);
    if (rc != RV_OK)
        return rc;
    cur = (rv_cursor *)calloc(1, sizeof(*cur));
    if (!cur)
        return RV_ERR_NOMEM;
    cur->sess = sess;
    if (rv_cursor_impl_open(&cur->impl, &sess->pkg->parser.result, &sess->index,
                            &sess->cache) != 0) {
        free(cur);
        return RV_ERR_INTERNAL;
    }
    *out = cur;
    return RV_OK;
}

rv_status rv_cursor_seek_window(rv_cursor *cur, uint32_t window_id)
{
    if (!cur)
        return RV_ERR_INVALID_ARG;
    return rv_cursor_impl_seek(&cur->impl, window_id) == 0 ? RV_OK
                                                           : RV_ERR_NOT_FOUND;
}

rv_status rv_cursor_read_wave(rv_cursor *cur, rv_wave_slice *slice)
{
    if (!cur)
        return RV_ERR_INVALID_ARG;
    return rv_cursor_impl_read(&cur->impl, slice) == 0 ? RV_OK : RV_ERR_CURSOR;
}

rv_status rv_cursor_advance(rv_cursor *cur)
{
    if (!cur)
        return RV_ERR_INVALID_ARG;
    return rv_cursor_impl_advance(&cur->impl) == 0 ? RV_OK : RV_ERR_NOT_FOUND;
}

void rv_cursor_close(rv_cursor *cur)
{
    if (!cur)
        return;
    rv_cursor_impl_close(&cur->impl);
    free(cur);
}

/* ---------------- checkpoint / export / repair ---------------- */

rv_status rv_checkpoint_create(rv_checkpoint **out, rv_session *sess)
{
    rv_checkpoint *cp;
    rv_status st;
    if (!out || !sess)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index(sess);
    if (st != RV_OK)
        return st;
    if (!sess->derived.valid) {
        if (rv_derived_summary_build(&sess->derived, &sess->pkg->arena,
                                     &sess->pkg->parser.result,
                                     &sess->index) != 0)
            return RV_ERR_NOMEM;
    }
    cp = (rv_checkpoint *)calloc(1, sizeof(*cp));
    if (!cp)
        return RV_ERR_NOMEM;
    cp->sess = sess;
    if (rv_checkpoint_capture(&cp->impl, &sess->index, &sess->derived) != 0) {
        free(cp);
        return RV_ERR_CHECKPOINT;
    }
    *out = cp;
    return RV_OK;
}

rv_status rv_checkpoint_restore(rv_session *sess, rv_checkpoint *cp)
{
    if (!sess || !cp || !cp->impl.valid)
        return RV_ERR_INVALID_ARG;
    if (rv_checkpoint_apply_summary(&cp->impl, &sess->derived) != 0)
        return RV_ERR_CHECKPOINT;
    return RV_OK;
}

void rv_checkpoint_destroy(rv_checkpoint *cp)
{
    if (!cp)
        return;
    rv_checkpoint_impl_clear(&cp->impl);
    free(cp);
}

rv_status rv_export_buffer(rv_session *sess, uint32_t flags,
                           uint8_t **out_buf, size_t *out_size)
{
    rv_buf buf;
    rv_status st;
    if (!sess || !out_buf || !out_size)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index(sess);
    if (st != RV_OK)
        return st;
    if (!sess->derived.valid)
        (void)rv_derived_summary_build(&sess->derived, &sess->pkg->arena,
                                       &sess->pkg->parser.result,
                                       &sess->index);
    if (rv_buf_init(&buf, 1024) != 0)
        return RV_ERR_NOMEM;
    if (rv_export_build(&sess->pkg->parser.result, &sess->index,
                        &sess->derived, flags, &buf) != 0) {
        rv_buf_release(&buf);
        return RV_ERR_EXPORT;
    }
    *out_buf = buf.data;
    *out_size = buf.size;
    /* Caller owns the buffer; detach without free. */
    buf.data = NULL;
    buf.owned = 0;
    rv_buf_release(&buf);
    rv_notify_emit(&sess->notify, RV_EVENT_EXPORT, "export complete");
    return RV_OK;
}

void rv_export_free(uint8_t *buf)
{
    free(buf);
}

rv_status rv_package_repair(rv_package *pkg, uint32_t repair_flags)
{
    rv_repair_options o;
    if (!pkg || !pkg->opened)
        return RV_ERR_STATE;
    rv_repair_options_default(&o);
    if (repair_flags & 1)
        o.synthesize_missing_names = 1;
    if (rv_repair_package(&pkg->reader, &pkg->parser.result, &pkg->arena, &o,
                          &pkg->diag) != 0)
        return RV_ERR_REPAIR;
    return RV_OK;
}

rv_status rv_package_migrate_legacy(rv_package *pkg)
{
    if (!pkg || !pkg->opened)
        return RV_ERR_STATE;
    if (rv_legacy_migrate(&pkg->reader, &pkg->parser.result, &pkg->arena,
                          &pkg->diag) != 0)
        return RV_ERR_LEGACY;
    /*
     * Migration replaces package backing in place. Parsed WAVE sample
     * views continue to reference the prior buffer until the caller
     * invokes rv_package_reset() or rebuilds a session index from a
     * fresh parse. Monitoring tools historically relied on this lazy
     * refresh so warm caches stayed populated across format bumps.
     */
    pkg->legacy = 0;
    return RV_OK;
}
