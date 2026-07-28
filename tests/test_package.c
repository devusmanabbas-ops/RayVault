#include "rayvault/rayvault.h"
#include "rv_builder.h"

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

static uint8_t *make_pkg(size_t *out_sz, uint32_t samples)
{
    rv_builder b;
    rv_buf buf;
    uint8_t *owned;
    if (rv_builder_init(&b) != 0)
        return NULL;
    if (rv_builder_demo_span(&b, "CO-North", "POP-12", 12.5f, samples) != 0) {
        rv_builder_destroy(&b);
        return NULL;
    }
    if (rv_buf_init(&buf, 1024) != 0) {
        rv_builder_destroy(&b);
        return NULL;
    }
    if (rv_builder_serialize(&b, &buf) != 0) {
        rv_buf_release(&buf);
        rv_builder_destroy(&b);
        return NULL;
    }
    owned = buf.data;
    *out_sz = buf.size;
    buf.data = NULL;
    buf.owned = 0;
    rv_buf_release(&buf);
    rv_builder_destroy(&b);
    return owned;
}

static void test_valid_parse(void)
{
    size_t sz = 0;
    uint8_t *data = make_pkg(&sz, 256);
    rv_package *pkg = NULL;
    rv_package_info info;
    expect(data != NULL, "builder produced package");
    expect(rv_package_open_memory(&pkg, data, sz, RV_OPEN_BUILD_INDEX, NULL) ==
               RV_OK,
           "open memory");
    expect(rv_package_get_info(pkg, &info) == RV_OK, "info");
    expect(info.route_count == 1, "one route");
    expect(info.window_count == 1, "one window");
    expect(info.wave_block_count == 1, "one wave");
    expect(rv_package_validate(pkg, 1) == RV_OK, "validate");
    rv_package_close(pkg);
    free(data);
}

static void test_malformed_reject(void)
{
    uint8_t junk[64];
    rv_package *pkg = NULL;
    memset(junk, 0xAB, sizeof(junk));
    expect(rv_package_open_memory(&pkg, junk, sizeof(junk), 0, NULL) != RV_OK,
           "reject garbage");
}

static void test_crossref_and_query(void)
{
    size_t sz = 0;
    uint8_t *data = make_pkg(&sz, 128);
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_marker_info marks[8];
    size_t n = 0;
    rv_query_filter flt;
    expect(rv_package_open_memory(&pkg, data, sz,
                                  RV_OPEN_BUILD_INDEX | RV_OPEN_WARM_CACHE,
                                  NULL) == RV_OK,
           "open");
    expect(rv_session_create(&sess, pkg) == RV_OK, "session");
    memset(&flt, 0, sizeof(flt));
    expect(rv_marker_query(sess, &flt, marks, 8, &n) == RV_OK, "query");
    expect(n >= 1, "marker found");
    rv_session_destroy(sess);
    rv_package_close(pkg);
    free(data);
}

static void test_export_roundtrip(void)
{
    size_t sz = 0, out_sz = 0;
    uint8_t *data = make_pkg(&sz, 64);
    uint8_t *exported = NULL;
    rv_package *pkg = NULL, *pkg2 = NULL;
    rv_session *sess = NULL;
    rv_package_info a, b;
    expect(rv_package_open_memory(&pkg, data, sz, RV_OPEN_BUILD_INDEX, NULL) ==
               RV_OK,
           "open");
    expect(rv_session_create(&sess, pkg) == RV_OK, "session");
    expect(rv_export_buffer(sess, RV_EXPORT_INCLUDE_WAVE | RV_EXPORT_WITH_SUMMARY,
                            &exported, &out_sz) == RV_OK,
           "export");
    expect(rv_package_open_memory(&pkg2, exported, out_sz, RV_OPEN_BUILD_INDEX,
                                  NULL) == RV_OK,
           "reopen exported");
    expect(rv_package_get_info(pkg, &a) == RV_OK && rv_package_get_info(pkg2, &b) == RV_OK,
           "infos");
    expect(a.route_count == b.route_count, "route roundtrip");
    expect(a.window_count == b.window_count, "window roundtrip");
    rv_export_free(exported);
    rv_session_destroy(sess);
    rv_package_close(pkg2);
    rv_package_close(pkg);
    free(data);
}

int main(void)
{
    test_valid_parse();
    test_malformed_reject();
    test_crossref_and_query();
    test_export_roundtrip();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_package: ok\n");
    return 0;
}
