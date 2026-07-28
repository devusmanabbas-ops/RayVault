#include "rayvault/rayvault.h"
#include "rv_builder.h"
#include "rv_session_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        failures++;
    }
}

static uint8_t *make_pkg(size_t *out_sz)
{
    rv_builder b;
    rv_buf buf;
    if (rv_builder_init(&b) != 0)
        return NULL;
    rv_builder_demo_span(&b, "A", "B", 8.0f, 200);
    rv_buf_init(&buf, 512);
    rv_builder_serialize(&b, &buf);
    *out_sz = buf.size;
    {
        uint8_t *p = buf.data;
        buf.data = NULL;
        buf.owned = 0;
        rv_buf_release(&buf);
        rv_builder_destroy(&b);
        return p;
    }
}

static void test_rebuild_export(void)
{
    size_t sz = 0, out_sz = 0;
    uint8_t *data = make_pkg(&sz);
    uint8_t *out = NULL;
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_config cfg;
    rv_config_init(&cfg);
    cfg.arena_initial_bytes = 4096;
    cfg.cache_capacity = 4;
    expect(rv_package_open_memory(&pkg, data, sz,
                                  RV_OPEN_BUILD_INDEX | RV_OPEN_WARM_CACHE,
                                  &cfg) == RV_OK,
           "open");
    expect(rv_session_create(&sess, pkg) == RV_OK, "session");
    expect(rv_session_warm_cache(sess) == RV_OK, "warm");
    expect(rv_session_export_after_rebuild(
               sess, RV_EXPORT_INCLUDE_WAVE | RV_EXPORT_WITH_SUMMARY, &out,
               &out_sz) == RV_OK,
           "export after rebuild");
    expect(out && out_sz > 64, "exported bytes");
    rv_export_free(out);
    rv_session_destroy(sess);
    rv_package_close(pkg);
    free(data);
}

static void test_reset_rebind(void)
{
    size_t sz = 0;
    uint8_t *data = make_pkg(&sz);
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_wave_slice sl;
    expect(rv_package_open_memory(&pkg, data, sz,
                                  RV_OPEN_BUILD_INDEX | RV_OPEN_WARM_CACHE,
                                  NULL) == RV_OK,
           "open");
    expect(rv_session_create(&sess, pkg) == RV_OK, "session");
    expect(rv_wave_get(sess, 1, &sl) == RV_OK, "wave before reset");
    expect(rv_package_reset(pkg) == RV_OK, "reset");
    expect(rv_session_rebind(sess, 0) == RV_OK, "rebind clear cache");
    expect(rv_wave_get(sess, 1, &sl) == RV_OK, "wave after rebind");
    rv_session_destroy(sess);
    rv_package_close(pkg);
    free(data);
}

static void test_repair(void)
{
    size_t sz = 0;
    uint8_t *data = make_pkg(&sz);
    rv_package *pkg = NULL;
    expect(rv_package_open_memory(&pkg, data, sz, RV_OPEN_BUILD_INDEX, NULL) ==
               RV_OK,
           "open");
    expect(rv_package_repair(pkg, 0) == RV_OK, "repair");
    rv_package_close(pkg);
    free(data);
}

int main(void)
{
    test_rebuild_export();
    test_reset_rebind();
    test_repair();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_lifecycle: ok\n");
    return 0;
}
