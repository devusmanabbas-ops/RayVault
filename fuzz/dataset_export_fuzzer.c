#include "rayvault/rayvault.h"
#include "rv_session_ops.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * Builds derived metadata, rebuilds indexes, then serializes the package
 * for downstream exchange.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_checkpoint *cp = NULL;
    rv_config cfg;
    uint8_t *exported = NULL;
    size_t exported_sz = 0;
    rv_stats_snapshot snap;

    if (size < 64 || size > 4u * 1024u * 1024u)
        return 0;

    rv_config_init(&cfg);
    cfg.arena_initial_bytes = 6 * 1024;
    cfg.arena_max_bytes = 12u * 1024u * 1024u;
    cfg.cache_capacity = 4;
    cfg.log_level = 0;

    if (rv_package_open_memory(&pkg, data, size,
                               RV_OPEN_BUILD_INDEX | RV_OPEN_WARM_CACHE |
                                   RV_OPEN_ALLOW_LEGACY,
                               &cfg) != RV_OK)
        return 0;
    if (rv_session_create(&sess, pkg) != RV_OK) {
        rv_package_close(pkg);
        return 0;
    }

    (void)rv_session_warm_cache(sess);
    (void)rv_session_stats(sess, &snap);
    (void)rv_checkpoint_create(&cp, sess);

    (void)rv_session_export_after_rebuild(
        sess, RV_EXPORT_INCLUDE_WAVE | RV_EXPORT_WITH_SUMMARY, &exported,
        &exported_sz);

    if (cp) {
        (void)rv_checkpoint_restore(sess, cp);
        rv_checkpoint_destroy(cp);
    }

    if (exported) {
        /* Optionally re-open the exported image on the same path. */
        rv_package *pkg2 = NULL;
        if (rv_package_open_memory(&pkg2, exported, exported_sz,
                                   RV_OPEN_BUILD_INDEX, &cfg) == RV_OK) {
            rv_package_close(pkg2);
        }
        rv_export_free(exported);
    }

    rv_session_destroy(sess);
    rv_package_close(pkg);
    return 0;
}
