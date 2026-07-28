#include "rayvault/rayvault.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Opens a package, warms the wave cache, then streams and seeks with
 * cursors to exercise eviction and reuse.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_stream *st = NULL;
    rv_cursor *cur = NULL;
    rv_config cfg;
    uint32_t windex = 0;
    rv_wave_slice slice;
    uint32_t windows = 0;
    int i;

    if (size < 64 || size > 4u * 1024u * 1024u)
        return 0;

    rv_config_init(&cfg);
    cfg.arena_initial_bytes = 8 * 1024;
    cfg.cache_capacity = 2; /* pressure LRU */
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
    (void)rv_window_count(sess, &windows);

    if (rv_stream_open(&st, sess, 0) == RV_OK) {
        for (i = 0; i < 16; i++) {
            if (rv_stream_next_window(st, &windex) != RV_OK)
                break;
            memset(&slice, 0, sizeof(slice));
            (void)rv_stream_next_wave(st, &slice);
            if (slice.samples && slice.count)
                (void)slice.samples[0];
        }
        rv_stream_close(st);
    }

    if (rv_cursor_open(&cur, sess) == RV_OK) {
        uint32_t wid = 1;
        rv_window_key key;
        float pulse = 0;
        uint32_t wave_id = 0;
        if (windows > 0 && rv_window_get(sess, 0, &key, &pulse, &wave_id) == RV_OK)
            wid = key.window_id;
        if (rv_cursor_seek_window(cur, wid) == RV_OK) {
            memset(&slice, 0, sizeof(slice));
            (void)rv_cursor_read_wave(cur, &slice);
            if (slice.samples && slice.count > 1)
                (void)slice.samples[slice.count / 2];
            (void)rv_cursor_advance(cur);
            (void)rv_cursor_read_wave(cur, &slice);
        }
        rv_cursor_close(cur);
    }

    rv_session_destroy(sess);
    rv_package_close(pkg);
    return 0;
}
