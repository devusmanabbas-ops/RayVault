#include "rv_session_ops.h"
#include "rv_export.h"

#include <string.h>

rv_status rv_session_run_query(rv_session *sess, const rv_query_plan *plan,
                               rv_query_result *out)
{
    rv_status st;
    if (!sess || !plan || !out)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index(sess);
    if (st != RV_OK)
        return st;
    if (rv_query_execute(&sess->pkg->arena, &sess->pkg->parser.result,
                         &sess->index, plan, out) != 0)
        return RV_ERR_INTERNAL;
    return RV_OK;
}

rv_status rv_session_plant_report(rv_session *sess, rv_plant_report *out)
{
    rv_status st;
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    st = rv_session_ensure_index(sess);
    if (st != RV_OK)
        return st;
    if (rv_analytics_route_health(&sess->pkg->arena, &sess->pkg->parser.result,
                                  &sess->index, out) != 0)
        return RV_ERR_INTERNAL;
    return RV_OK;
}

rv_status rv_session_wave_stats(rv_session *sess, uint32_t wave_block_id,
                                rv_wave_stats *out)
{
    rv_wave_slice sl;
    rv_status st;
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    st = rv_wave_get(sess, wave_block_id, &sl);
    if (st != RV_OK)
        return st;
    return rv_wave_compute_stats(&sl, out) == 0 ? RV_OK : RV_ERR_INTERNAL;
}

rv_status rv_session_acq_context(rv_session *sess, uint32_t window_id,
                                 rv_acq_context *out)
{
    if (!sess || !out)
        return RV_ERR_INVALID_ARG;
    return rv_acq_context_for_window(&sess->pkg->parser.result, window_id,
                                     out) == 0
               ? RV_OK
               : RV_ERR_NOT_FOUND;
}

rv_status rv_session_rebind(rv_session *sess, int retain_cache)
{
    if (!sess || !sess->pkg || !sess->pkg->opened)
        return RV_ERR_STATE;

    sess->index_ready = 0;
    rv_index_clear(&sess->index);

    if (!retain_cache) {
        rv_cache_invalidate(&sess->cache);
        rv_derived_summary_invalidate(&sess->derived);
    }
    /*
     * When retain_cache is set, existing cache entries keep sample
     * pointers established before rebind. If the package backing was
     * replaced (reopen / migrate), those pointers refer to prior storage.
     */

    {
        rv_status st = rv_session_ensure_index(sess);
        if (st != RV_OK)
            return st;
    }
    rv_notify_emit(&sess->notify, RV_EVENT_REOPEN, "session rebound");
    return RV_OK;
}

rv_status rv_session_export_after_rebuild(rv_session *sess, uint32_t flags,
                                          uint8_t **buf, size_t *n)
{
    rv_status st;
    if (!sess)
        return RV_ERR_INVALID_ARG;

    /* Capture derived snapshot, rebuild index, then export using snapshot. */
    if (!sess->derived.valid) {
        st = rv_session_ensure_index(sess);
        if (st != RV_OK)
            return st;
        if (rv_derived_summary_build(&sess->derived, &sess->pkg->arena,
                                     &sess->pkg->parser.result,
                                     &sess->index) != 0)
            return RV_ERR_NOMEM;
    }

    st = rv_session_rebuild_index(sess);
    if (st != RV_OK)
        return st;

    return rv_export_buffer(sess, flags, buf, n);
}
