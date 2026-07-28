#include "rayvault/rayvault.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_package_info info;
    rv_stats_snapshot stats;
    uint32_t i, routes = 0, windows = 0;
    rv_status st;
    rv_marker_info marks[64];
    size_t n = 0;
    rv_query_filter flt;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.rvp>\n", argv[0]);
        return 2;
    }

    st = rv_package_open_file(&pkg, argv[1],
                              RV_OPEN_BUILD_INDEX | RV_OPEN_WARM_CACHE |
                                  RV_OPEN_ALLOW_LEGACY,
                              NULL);
    if (st != RV_OK) {
        fprintf(stderr, "open failed: %s (%s)\n", rv_status_string(st),
                rv_package_last_error(pkg));
        return 1;
    }

    rv_package_get_info(pkg, &info);
    printf("format %u.%u  sections=%u  bytes=%llu\n", info.format.major,
           info.format.minor, info.section_count,
           (unsigned long long)info.package_bytes);
    printf("routes=%u windows=%u markers=%u waves=%u\n", info.route_count,
           info.window_count, info.marker_count, info.wave_block_count);

    if (rv_session_create(&sess, pkg) != RV_OK) {
        rv_package_close(pkg);
        return 1;
    }

    rv_session_stats(sess, &stats);
    printf("mean_span_km=%.3f max_loss_db=%.3f total_samples=%llu\n",
           stats.mean_span_km, stats.max_loss_db,
           (unsigned long long)stats.total_samples);

    rv_route_count(sess, &routes);
    for (i = 0; i < routes && i < 16; i++) {
        uint32_t rid = 0, sid = 0, fibers = 0;
        float km = 0;
        const char *name = NULL;
        size_t nlen = 0;
        rv_route_at(sess, i, &rid, &sid, &km, &fibers);
        (void)rv_name_lookup(sess, sid, &name, &nlen);
        printf("  route %u  %.3f km  fibers=%u  site=%.*s\n", rid, km, fibers,
               (int)nlen, name ? name : "");
    }

    memset(&flt, 0, sizeof(flt));
    if (rv_marker_query(sess, &flt, marks, 64, &n) == RV_OK) {
        size_t k;
        printf("markers (showing up to %zu of %zu):\n", n > 16 ? 16 : n, n);
        for (k = 0; k < n && k < 16; k++) {
            printf("  id=%u win=%u dist=%.1fm kind=%u sev=%u\n",
                   marks[k].marker_id, marks[k].window_id, marks[k].distance_m,
                   marks[k].kind, marks[k].severity);
        }
    }

    (void)windows;
    rv_session_destroy(sess);
    rv_package_close(pkg);
    return 0;
}
