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

static uint8_t *make_multi_wave(size_t *out_sz)
{
    rv_builder b;
    rv_buf buf;
    uint32_t site = 0, far = 0, model = 0, serial = 0;
    rv_builder_route route;
    rv_builder_inst inst;
    rv_builder_calib calib;
    int16_t samples[64];
    uint32_t i;

    if (rv_builder_init(&b) != 0)
        return NULL;
    rv_builder_add_name(&b, "Hub", &site);
    rv_builder_add_name(&b, "Edge", &far);
    rv_builder_add_name(&b, "Model", &model);
    rv_builder_add_name(&b, "Ser", &serial);
    memset(&route, 0, sizeof(route));
    route.route_id = 1;
    route.site_name_id = site;
    route.far_name_id = far;
    route.length_km = 5.0f;
    route.fiber_count = 24;
    rv_builder_add_route(&b, &route);
    memset(&inst, 0, sizeof(inst));
    inst.inst_id = 1;
    inst.model_name_id = model;
    inst.serial_name_id = serial;
    inst.wavelength_nm = 1310;
    inst.pulse_default_ns = 50;
    inst.dynamic_range_db = 35.0f;
    rv_builder_add_inst(&b, &inst);
    memset(&calib, 0, sizeof(calib));
    calib.calib_id = 1;
    calib.inst_id = 1;
    calib.calibrated_unix = 1600000000ull;
    calib.refractive_index = 1.4675f;
    calib.backscatter_coef = -80.0f;
    rv_builder_add_calib(&b, &calib);

    for (i = 0; i < 64; i++)
        samples[i] = (int16_t)(-1000 - (int)i);

    for (i = 1; i <= 6; i++) {
        rv_builder_window w;
        memset(&w, 0, sizeof(w));
        w.window_id = i;
        w.route_id = 1;
        w.inst_id = 1;
        w.calib_id = 1;
        w.wave_block_id = i;
        w.pulse_ns = 50.0f;
        w.range_km = 5.0f;
        w.resolution_m = 1.0f;
        w.acquired_unix = 1600000000ull + i;
        rv_builder_add_window(&b, &w);
        rv_builder_add_wave_copy(&b, i, i, samples, 64, 0.0f, 1.0f);
    }

    rv_buf_init(&buf, 1024);
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

static void test_stream_and_eviction(void)
{
    size_t sz = 0;
    uint8_t *data = make_multi_wave(&sz);
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    rv_stream *st = NULL;
    rv_cursor *cur = NULL;
    rv_config cfg;
    uint32_t windex;
    rv_wave_slice sl;
    int waves = 0;

    rv_config_init(&cfg);
    cfg.cache_capacity = 2; /* force eviction */

    expect(rv_package_open_memory(&pkg, data, sz, RV_OPEN_BUILD_INDEX, &cfg) ==
               RV_OK,
           "open");
    expect(rv_session_create(&sess, pkg) == RV_OK, "session");
    expect(rv_stream_open(&st, sess, 1) == RV_OK, "stream");
    while (rv_stream_next_window(st, &windex) == RV_OK) {
        if (rv_stream_next_wave(st, &sl) == RV_OK)
            waves++;
    }
    expect(waves >= 2, "streamed waves");
    rv_stream_close(st);

    expect(rv_cursor_open(&cur, sess) == RV_OK, "cursor");
    expect(rv_cursor_seek_window(cur, 1) == RV_OK, "seek");
    expect(rv_cursor_read_wave(cur, &sl) == RV_OK, "read");
    expect(sl.count == 64, "sample count");
    (void)rv_cursor_advance(cur);
    rv_cursor_close(cur);

    rv_session_destroy(sess);
    rv_package_close(pkg);
    free(data);
}

int main(void)
{
    test_stream_and_eviction();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_stream_cache: ok\n");
    return 0;
}
