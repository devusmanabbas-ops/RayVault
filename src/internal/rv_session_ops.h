#ifndef RV_SESSION_OPS_H
#define RV_SESSION_OPS_H

#include "rv_package.h"
#include "rv_analytics.h"
#include "rv_query.h"
#include "rv_waveproc.h"
#include "rv_acqmeta.h"

/*
 * Extended session operations used by tools and fuzz harnesses.
 * These intentionally compose multiple subsystems on a live session.
 */

rv_status rv_session_run_query(rv_session *sess, const rv_query_plan *plan,
                               rv_query_result *out);
rv_status rv_session_plant_report(rv_session *sess, rv_plant_report *out);
rv_status rv_session_wave_stats(rv_session *sess, uint32_t wave_block_id,
                                rv_wave_stats *out);
rv_status rv_session_acq_context(rv_session *sess, uint32_t window_id,
                                 rv_acq_context *out);

/*
 * Rebind session after package reopen/reset. Index is rebuilt; wave cache
 * entries are retained when retain_cache is non-zero so streaming clients
 * can continue without a full warm-up. Callers must ensure block ids still
 * match if retain_cache is used.
 */
rv_status rv_session_rebind(rv_session *sess, int retain_cache);

rv_status rv_session_export_after_rebuild(rv_session *sess, uint32_t flags,
                                          uint8_t **buf, size_t *n);

#endif /* RV_SESSION_OPS_H */
