#include "rayvault/rayvault.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Exercises package open, session creation, validation, and basic
 * metadata inspection on a single loaded object.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_config cfg;
    rv_package_info info;
    uint32_t routes = 0;

    if (size < 64 || size > 4u * 1024u * 1024u)
        return 0;

    rv_config_init(&cfg);
    cfg.arena_initial_bytes = 8 * 1024;
    cfg.arena_max_bytes = 8u * 1024u * 1024u;
    cfg.cache_capacity = 4;
    cfg.log_level = 0;

    if (rv_package_open_memory(&pkg, data, size,
                               RV_OPEN_BUILD_INDEX | RV_OPEN_ALLOW_LEGACY |
                                   RV_OPEN_WARM_CACHE,
                               &cfg) != RV_OK)
        return 0;

    (void)rv_package_get_info(pkg, &info);
    (void)rv_package_validate(pkg, 0);

    if (rv_session_create(&sess, pkg) == RV_OK) {
        (void)rv_route_count(sess, &routes);
        (void)rv_session_warm_cache(sess);
        (void)rv_session_stats(sess, NULL);
        {
            rv_stats_snapshot snap;
            (void)rv_session_stats(sess, &snap);
        }
        rv_session_destroy(sess);
    }

    rv_package_close(pkg);
    return 0;
}
