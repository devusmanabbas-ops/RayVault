#include "rayvault/rayvault.h"
#include "rv_session_ops.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Applies repair / legacy migration, rebinds the session, and re-reads
 * wave data through cache and cursor paths.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_cursor *cur = NULL;
    rv_config cfg;
    rv_wave_slice slice;
    uint32_t windows = 0;
    int retain;

    if (size < 64 || size > 4u * 1024u * 1024u)
        return 0;

    rv_config_init(&cfg);
    cfg.arena_initial_bytes = 5 * 1024;
    cfg.cache_capacity = 3;
    cfg.log_level = 0;

    if (rv_package_open_memory(&pkg, data, size,
                               RV_OPEN_BUILD_INDEX | RV_OPEN_WARM_CACHE |
                                   RV_OPEN_ALLOW_LEGACY | RV_OPEN_REPAIR_HINTS,
                               &cfg) != RV_OK)
        return 0;
    if (rv_session_create(&sess, pkg) != RV_OK) {
        rv_package_close(pkg);
        return 0;
    }

    (void)rv_session_warm_cache(sess);
    (void)rv_window_count(sess, &windows);

    if (windows > 0) {
        rv_window_key key;
        float pulse = 0;
        uint32_t wave_id = 0;
        if (rv_window_get(sess, 0, &key, &pulse, &wave_id) == RV_OK)
            (void)rv_wave_get(sess, wave_id, &slice);
    }

    (void)rv_package_repair(pkg, data[size - 1] & 1u);
    (void)rv_package_migrate_legacy(pkg);

    retain = (int)(data[size / 2] & 1u);
    (void)rv_session_rebind(sess, retain);

    if (rv_cursor_open(&cur, sess) == RV_OK) {
        if (windows > 0) {
            rv_window_key key;
            float pulse = 0;
            uint32_t wave_id = 0;
            if (rv_window_get(sess, 0, &key, &pulse, &wave_id) == RV_OK) {
                if (rv_cursor_seek_window(cur, key.window_id) == RV_OK) {
                    memset(&slice, 0, sizeof(slice));
                    (void)rv_cursor_read_wave(cur, &slice);
                    if (slice.samples && slice.count)
                        (void)slice.samples[0];
                }
            }
        }
        rv_cursor_close(cur);
    }

    (void)rv_package_reset(pkg);
    (void)rv_session_rebind(sess, 1);
    (void)rv_session_warm_cache(sess);

    rv_session_destroy(sess);
    rv_package_close(pkg);
    return 0;
}
