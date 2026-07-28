#include "rv_batch.h"

#include <stdio.h>
#include <string.h>

void rv_batch_options_default(rv_batch_options *o)
{
    if (!o)
        return;
    memset(o, 0, sizeof(*o));
    o->open_flags = RV_OPEN_BUILD_INDEX | RV_OPEN_ALLOW_LEGACY;
    o->validate = 1;
    o->warm_cache = 0;
    o->export_summary = 0;
    o->max_items = 1024;
}

static int rv_batch_fill_item_from_pkg(rv_batch_item *it, rv_package *pkg,
                                       const rv_batch_options *opt)
{
    rv_package_info info;
    rv_session *sess = NULL;
    rv_stats_snapshot snap;
    if (rv_package_get_info(pkg, &info) != RV_OK)
        return -1;
    it->routes = info.route_count;
    it->windows = info.window_count;
    it->markers = info.marker_count;
    it->bytes = info.package_bytes;
    if (opt->validate)
        (void)rv_package_validate(pkg, 0);
    if (rv_session_create(&sess, pkg) == RV_OK) {
        if (opt->warm_cache)
            (void)rv_session_warm_cache(sess);
        if (rv_session_stats(sess, &snap) == RV_OK)
            it->mean_span_km = snap.mean_span_km;
        rv_session_destroy(sess);
    }
    return 0;
}

int rv_batch_process_paths(rv_arena *a, const char **paths, size_t npaths,
                           const rv_batch_options *opt, rv_batch_result *out)
{
    rv_batch_options def;
    size_t i, n;
    if (!a || !paths || !out)
        return -1;
    if (!opt) {
        rv_batch_options_default(&def);
        opt = &def;
    }
    memset(out, 0, sizeof(*out));
    n = npaths;
    if (opt->max_items && n > opt->max_items)
        n = opt->max_items;
    out->items = (rv_batch_item *)rv_arena_calloc(a, n, sizeof(rv_batch_item), 8);
    if (!out->items && n)
        return -1;
    out->count = n;
    for (i = 0; i < n; i++) {
        rv_package *pkg = NULL;
        rv_batch_item *it = &out->items[i];
        rv_status st;
        strncpy(it->path, paths[i] ? paths[i] : "", sizeof(it->path) - 1);
        st = rv_package_open_file(&pkg, it->path, opt->open_flags, NULL);
        it->status = st;
        if (st != RV_OK) {
            out->err_count++;
            continue;
        }
        if (rv_batch_fill_item_from_pkg(it, pkg, opt) != 0) {
            it->status = RV_ERR_INTERNAL;
            out->err_count++;
        } else {
            out->ok_count++;
        }
        rv_package_close(pkg);
    }
    return 0;
}

int rv_batch_process_memory(rv_arena *a, const uint8_t **blobs,
                            const size_t *sizes, size_t n,
                            const rv_batch_options *opt, rv_batch_result *out)
{
    rv_batch_options def;
    size_t i, lim;
    if (!a || !blobs || !sizes || !out)
        return -1;
    if (!opt) {
        rv_batch_options_default(&def);
        opt = &def;
    }
    memset(out, 0, sizeof(*out));
    lim = n;
    if (opt->max_items && lim > opt->max_items)
        lim = opt->max_items;
    out->items = (rv_batch_item *)rv_arena_calloc(a, lim, sizeof(rv_batch_item), 8);
    if (!out->items && lim)
        return -1;
    out->count = lim;
    for (i = 0; i < lim; i++) {
        rv_package *pkg = NULL;
        rv_batch_item *it = &out->items[i];
        rv_status st;
        snprintf(it->path, sizeof(it->path), "mem[%zu]", i);
        st = rv_package_open_memory(&pkg, blobs[i], sizes[i], opt->open_flags,
                                    NULL);
        it->status = st;
        if (st != RV_OK) {
            out->err_count++;
            continue;
        }
        if (rv_batch_fill_item_from_pkg(it, pkg, opt) != 0) {
            it->status = RV_ERR_INTERNAL;
            out->err_count++;
        } else {
            out->ok_count++;
        }
        rv_package_close(pkg);
    }
    return 0;
}

int rv_batch_summary_buffer(const rv_batch_result *res, rv_buf *out)
{
    size_t i;
    char line[256];
    int n;
    if (!res || !out)
        return -1;
    n = snprintf(line, sizeof(line), "batch items=%zu ok=%zu err=%zu\n",
                 res->count, res->ok_count, res->err_count);
    if (n < 0 || rv_buf_append(out, line, (size_t)n) != 0)
        return -1;
    for (i = 0; i < res->count; i++) {
        const rv_batch_item *it = &res->items[i];
        n = snprintf(line, sizeof(line),
                     "%s status=%d routes=%u windows=%u markers=%u bytes=%llu "
                     "mean_km=%.3f\n",
                     it->path, (int)it->status, it->routes, it->windows,
                     it->markers, (unsigned long long)it->bytes,
                     it->mean_span_km);
        if (n < 0 || rv_buf_append(out, line, (size_t)n) != 0)
            return -1;
    }
    return 0;
}
