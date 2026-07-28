#include "rayvault/rayvault.h"
#include "rv_session_ops.h"
#include "rv_query.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Builds indexes and runs filtered marker / plant queries against a
 * loaded monitoring package.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_config cfg;
    rv_query_plan plan;
    rv_query_result result;
    rv_marker_info marks[32];
    size_t n = 0;
    rv_query_filter flt;
    rv_plant_report report;

    if (size < 64 || size > 4u * 1024u * 1024u)
        return 0;

    rv_config_init(&cfg);
    cfg.arena_initial_bytes = 4 * 1024;
    cfg.arena_max_bytes = 16u * 1024u * 1024u;
    cfg.cache_capacity = 8;
    cfg.log_level = 0;

    if (rv_package_open_memory(&pkg, data, size,
                               RV_OPEN_BUILD_INDEX | RV_OPEN_ALLOW_LEGACY,
                               &cfg) != RV_OK)
        return 0;
    if (rv_session_create(&sess, pkg) != RV_OK) {
        rv_package_close(pkg);
        return 0;
    }

    memset(&flt, 0, sizeof(flt));
    if (size > 80) {
        flt.route_id = data[64] % 8;
        flt.min_severity = data[65] % 50;
        flt.marker_kind = data[66] % 8;
    }
    (void)rv_marker_query(sess, &flt, marks, 32, &n);

    rv_query_plan_init(&plan);
    plan.filter = flt;
    plan.sort_by_distance = 1;
    plan.require_wave = (data[67 % size] & 1u);
    plan.limit = 64;
    (void)rv_session_run_query(sess, &plan, &result);

    memset(&report, 0, sizeof(report));
    (void)rv_session_plant_report(sess, &report);

    /* Second pass on the same session with a rebuilt index. */
    (void)rv_session_rebuild_index(sess);
    (void)rv_marker_query(sess, &flt, marks, 32, &n);

    rv_session_destroy(sess);
    rv_package_close(pkg);
    return 0;
}
