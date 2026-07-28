#include "rv_builder.h"
#include "rv_crc.h"
#include "rv_endian.h"
#include "rv_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *rv_b_realloc_array(void *p, size_t n, size_t elem)
{
    return realloc(p, n * elem);
}

int rv_builder_init(rv_builder *b)
{
    if (!b)
        return -1;
    memset(b, 0, sizeof(*b));
    if (rv_arena_init(&b->arena, 32 * 1024, 32u * 1024u * 1024u) != 0)
        return -1;
    b->next_name_id = 1;
    b->ver_major = RV_FORMAT_VERSION_MAJOR;
    b->ver_minor = RV_FORMAT_VERSION_MINOR;
    b->include_summ = 1;
    b->include_notes = 1;
    return 0;
}

void rv_builder_destroy(rv_builder *b)
{
    uint32_t i;
    if (!b)
        return;
    for (i = 0; i < b->wave_count; i++) {
        if (b->waves[i].samples_owned)
            free(b->waves[i].samples);
    }
    free(b->names);
    free(b->routes);
    free(b->insts);
    free(b->calibs);
    free(b->windows);
    free(b->waves);
    free(b->markers);
    free(b->links);
    free(b->notes);
    rv_arena_destroy(&b->arena);
    memset(b, 0, sizeof(*b));
}

void rv_builder_set_version(rv_builder *b, uint16_t major, uint16_t minor)
{
    if (!b)
        return;
    b->ver_major = major;
    b->ver_minor = minor;
}

void rv_builder_set_created(rv_builder *b, uint64_t unix_ts)
{
    if (b)
        b->created_unix = unix_ts;
}

#define RV_BUILDER_GROW(field, count, cap, type)                               \
    do {                                                                       \
        if ((count) >= (cap)) {                                                \
            uint32_t ncap = (cap) ? (cap) * 2u : 8u;                            \
            type *np = (type *)rv_b_realloc_array((field), ncap, sizeof(type)); \
            if (!np)                                                           \
                return -1;                                                     \
            (field) = np;                                                      \
            (cap) = ncap;                                                      \
        }                                                                      \
    } while (0)

int rv_builder_add_name(rv_builder *b, const char *str, uint32_t *out_id)
{
    size_t n;
    uint32_t i;
    if (!b || !str || !out_id)
        return -1;
    n = strlen(str);
    if (n > 127)
        n = 127;
    for (i = 0; i < b->name_count; i++) {
        if (b->names[i].len == n && memcmp(b->names[i].str, str, n) == 0) {
            *out_id = b->names[i].id;
            return 0;
        }
    }
    RV_BUILDER_GROW(b->names, b->name_count, b->name_cap, rv_builder_name);
    b->names[b->name_count].id = b->next_name_id++;
    memcpy(b->names[b->name_count].str, str, n);
    b->names[b->name_count].str[n] = '\0';
    b->names[b->name_count].len = (uint16_t)n;
    *out_id = b->names[b->name_count].id;
    b->name_count++;
    return 0;
}

int rv_builder_add_route(rv_builder *b, const rv_builder_route *r)
{
    if (!b || !r)
        return -1;
    RV_BUILDER_GROW(b->routes, b->route_count, b->route_cap, rv_builder_route);
    b->routes[b->route_count++] = *r;
    return 0;
}

int rv_builder_add_inst(rv_builder *b, const rv_builder_inst *i)
{
    if (!b || !i)
        return -1;
    RV_BUILDER_GROW(b->insts, b->inst_count, b->inst_cap, rv_builder_inst);
    b->insts[b->inst_count++] = *i;
    return 0;
}

int rv_builder_add_calib(rv_builder *b, const rv_builder_calib *c)
{
    if (!b || !c)
        return -1;
    RV_BUILDER_GROW(b->calibs, b->calib_count, b->calib_cap, rv_builder_calib);
    b->calibs[b->calib_count++] = *c;
    return 0;
}

int rv_builder_add_window(rv_builder *b, const rv_builder_window *w)
{
    if (!b || !w)
        return -1;
    RV_BUILDER_GROW(b->windows, b->window_count, b->window_cap, rv_builder_window);
    b->windows[b->window_count++] = *w;
    return 0;
}

int rv_builder_add_wave(rv_builder *b, const rv_builder_wave *w)
{
    if (!b || !w)
        return -1;
    RV_BUILDER_GROW(b->waves, b->wave_count, b->wave_cap, rv_builder_wave);
    b->waves[b->wave_count] = *w;
    b->waves[b->wave_count].samples_owned = 0;
    b->wave_count++;
    return 0;
}

int rv_builder_add_wave_copy(rv_builder *b, uint32_t block_id,
                             uint32_t window_id, const int16_t *samples,
                             uint32_t count, float start_m, float step_m)
{
    rv_builder_wave w;
    int16_t *copy;
    if (!b || !samples || count == 0)
        return -1;
    copy = (int16_t *)malloc((size_t)count * sizeof(int16_t));
    if (!copy)
        return -1;
    memcpy(copy, samples, (size_t)count * sizeof(int16_t));
    memset(&w, 0, sizeof(w));
    w.block_id = block_id;
    w.window_id = window_id;
    w.sample_count = count;
    w.start_m = start_m;
    w.step_m = step_m;
    w.samples = copy;
    RV_BUILDER_GROW(b->waves, b->wave_count, b->wave_cap, rv_builder_wave);
    b->waves[b->wave_count] = w;
    b->waves[b->wave_count].samples_owned = 1;
    b->wave_count++;
    return 0;
}

int rv_builder_add_marker(rv_builder *b, const rv_builder_marker *m)
{
    if (!b || !m)
        return -1;
    RV_BUILDER_GROW(b->markers, b->marker_count, b->marker_cap, rv_builder_marker);
    b->markers[b->marker_count++] = *m;
    return 0;
}

int rv_builder_add_link(rv_builder *b, const rv_builder_link *l)
{
    if (!b || !l)
        return -1;
    RV_BUILDER_GROW(b->links, b->link_count, b->link_cap, rv_builder_link);
    b->links[b->link_count++] = *l;
    return 0;
}

int rv_builder_add_note(rv_builder *b, const rv_builder_note *n)
{
    if (!b || !n)
        return -1;
    RV_BUILDER_GROW(b->notes, b->note_count, b->note_cap, rv_builder_note);
    b->notes[b->note_count++] = *n;
    return 0;
}

static int rv_b_append_snam(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x314D414Eu) != 0 ||
        rv_buf_append_u32(body, b->name_count) != 0)
        return -1;
    for (i = 0; i < b->name_count; i++) {
        if (rv_buf_append_u32(body, b->names[i].id) != 0 ||
            rv_buf_append_u16(body, b->names[i].len) != 0 ||
            rv_buf_append(body, b->names[i].str, b->names[i].len) != 0)
            return -1;
        if ((b->names[i].len & 1u) && rv_buf_append_u8(body, 0) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_rout(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31544F52u) != 0 ||
        rv_buf_append_u32(body, b->route_count) != 0)
        return -1;
    for (i = 0; i < b->route_count; i++) {
        const rv_builder_route *r = &b->routes[i];
        if (rv_buf_append_u32(body, r->route_id) != 0 ||
            rv_buf_append_u32(body, r->site_name_id) != 0 ||
            rv_buf_append_u32(body, r->far_name_id) != 0 ||
            rv_buf_append_f32(body, r->length_km) != 0 ||
            rv_buf_append_u32(body, r->fiber_count) != 0 ||
            rv_buf_append_u32(body, r->flags) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_inst(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x3154534Eu) != 0 ||
        rv_buf_append_u32(body, b->inst_count) != 0)
        return -1;
    for (i = 0; i < b->inst_count; i++) {
        const rv_builder_inst *r = &b->insts[i];
        if (rv_buf_append_u32(body, r->inst_id) != 0 ||
            rv_buf_append_u32(body, r->model_name_id) != 0 ||
            rv_buf_append_u32(body, r->serial_name_id) != 0 ||
            rv_buf_append_u16(body, r->wavelength_nm) != 0 ||
            rv_buf_append_u16(body, r->pulse_default_ns) != 0 ||
            rv_buf_append_f32(body, r->dynamic_range_db) != 0 ||
            rv_buf_append_u32(body, r->flags) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_clbr(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31424C43u) != 0 ||
        rv_buf_append_u32(body, b->calib_count) != 0)
        return -1;
    for (i = 0; i < b->calib_count; i++) {
        const rv_builder_calib *r = &b->calibs[i];
        if (rv_buf_append_u32(body, r->calib_id) != 0 ||
            rv_buf_append_u32(body, r->inst_id) != 0 ||
            rv_buf_append_u64(body, r->calibrated_unix) != 0 ||
            rv_buf_append_f32(body, r->refractive_index) != 0 ||
            rv_buf_append_f32(body, r->backscatter_coef) != 0 ||
            rv_buf_append_f32(body, r->splice_loss_db) != 0 ||
            rv_buf_append_u32(body, r->flags) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_wind(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31444E49u) != 0 ||
        rv_buf_append_u32(body, b->window_count) != 0)
        return -1;
    for (i = 0; i < b->window_count; i++) {
        const rv_builder_window *r = &b->windows[i];
        if (rv_buf_append_u32(body, r->window_id) != 0 ||
            rv_buf_append_u32(body, r->route_id) != 0 ||
            rv_buf_append_u32(body, r->inst_id) != 0 ||
            rv_buf_append_u32(body, r->calib_id) != 0 ||
            rv_buf_append_u32(body, r->wave_block_id) != 0 ||
            rv_buf_append_f32(body, r->pulse_ns) != 0 ||
            rv_buf_append_f32(body, r->range_km) != 0 ||
            rv_buf_append_f32(body, r->resolution_m) != 0 ||
            rv_buf_append_u64(body, r->acquired_unix) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_wave(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31455641u) != 0 ||
        rv_buf_append_u32(body, b->wave_count) != 0)
        return -1;
    for (i = 0; i < b->wave_count; i++) {
        const rv_builder_wave *w = &b->waves[i];
        size_t bytes = (size_t)w->sample_count * sizeof(int16_t);
        uint16_t pc = (uint16_t)(rv_crc32(w->samples, bytes) & 0xFFFFu);
        if (rv_buf_append_u32(body, w->block_id) != 0 ||
            rv_buf_append_u32(body, w->window_id) != 0 ||
            rv_buf_append_u32(body, w->sample_count) != 0 ||
            rv_buf_append_f32(body, w->start_m) != 0 ||
            rv_buf_append_f32(body, w->step_m) != 0 ||
            rv_buf_append_u16(body, w->flags) != 0 ||
            rv_buf_append_u16(body, pc) != 0 ||
            rv_buf_append(body, w->samples, bytes) != 0)
            return -1;
        if ((bytes & 1u) && rv_buf_append_u8(body, 0) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_mark(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x314B524Du) != 0 ||
        rv_buf_append_u32(body, b->marker_count) != 0)
        return -1;
    for (i = 0; i < b->marker_count; i++) {
        const rv_builder_marker *r = &b->markers[i];
        if (rv_buf_append_u32(body, r->marker_id) != 0 ||
            rv_buf_append_u32(body, r->window_id) != 0 ||
            rv_buf_append_u32(body, r->label_name_id) != 0 ||
            rv_buf_append_f32(body, r->distance_m) != 0 ||
            rv_buf_append_f32(body, r->loss_db) != 0 ||
            rv_buf_append_f32(body, r->reflectance_db) != 0 ||
            rv_buf_append_u16(body, r->kind) != 0 ||
            rv_buf_append_u16(body, r->severity) != 0 ||
            rv_buf_append_u32(body, r->flags) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_link(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x314B4E4Cu) != 0 ||
        rv_buf_append_u32(body, b->link_count) != 0)
        return -1;
    for (i = 0; i < b->link_count; i++) {
        const rv_builder_link *r = &b->links[i];
        if (rv_buf_append_u32(body, r->link_id) != 0 ||
            rv_buf_append_u32(body, r->from_kind) != 0 ||
            rv_buf_append_u32(body, r->from_id) != 0 ||
            rv_buf_append_u32(body, r->to_kind) != 0 ||
            rv_buf_append_u32(body, r->to_id) != 0 ||
            rv_buf_append_u32(body, r->flags) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_note(const rv_builder *b, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x3145544Eu) != 0 ||
        rv_buf_append_u32(body, b->note_count) != 0)
        return -1;
    for (i = 0; i < b->note_count; i++) {
        const rv_builder_note *r = &b->notes[i];
        if (rv_buf_append_u32(body, r->note_id) != 0 ||
            rv_buf_append_u32(body, r->target_kind) != 0 ||
            rv_buf_append_u32(body, r->target_id) != 0 ||
            rv_buf_append_u32(body, r->text_name_id) != 0 ||
            rv_buf_append_u32(body, r->flags) != 0)
            return -1;
    }
    return 0;
}

static int rv_b_append_summ(const rv_builder *b, rv_buf *body)
{
    if (rv_buf_append_u32(body, 0x314D4D55u) != 0 ||
        rv_buf_append_u32(body, b->route_count) != 0 ||
        rv_buf_append_u32(body, b->window_count) != 0 ||
        rv_buf_append_u32(body, b->marker_count) != 0 ||
        rv_buf_append_u32(body, b->wave_count) != 0 ||
        rv_buf_append_u32(body, b->name_count) != 0 ||
        rv_buf_append_u32(body, 0) != 0 ||
        rv_buf_append_u64(body, 0) != 0 ||
        rv_buf_append_u32(body, 0) != 0)
        return -1;
    return 0;
}

typedef struct rv_b_sec {
    uint32_t tag;
    rv_buf   body;
} rv_b_sec;

int rv_builder_serialize(rv_builder *b, rv_buf *out)
{
    rv_b_sec secs[12];
    rv_buf dir;
    uint32_t nsec = 0, i;
    uint32_t dir_offset;
    size_t payload_off;
    uint8_t hdr[RV_FILE_HEADER_SIZE];
    rv_file_header h;

    if (!b || !out)
        return -1;
    memset(secs, 0, sizeof(secs));
    if (rv_buf_init(&dir, 256) != 0)
        return -1;

#define RV_PUSH_SEC(tagv, fn)                                                  \
    do {                                                                       \
        secs[nsec].tag = (tagv);                                               \
        if (rv_buf_init(&secs[nsec].body, 128) != 0 || (fn) != 0)              \
            goto fail;                                                         \
        nsec++;                                                                \
    } while (0)

    RV_PUSH_SEC(RV_TAG_SNAM, rv_b_append_snam(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_ROUT, rv_b_append_rout(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_INST, rv_b_append_inst(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_CLBR, rv_b_append_clbr(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_WIND, rv_b_append_wind(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_WAVE, rv_b_append_wave(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_MARK, rv_b_append_mark(b, &secs[nsec].body));
    RV_PUSH_SEC(RV_TAG_LINK, rv_b_append_link(b, &secs[nsec].body));
    if (b->include_notes)
        RV_PUSH_SEC(RV_TAG_NOTE, rv_b_append_note(b, &secs[nsec].body));
    if (b->include_summ)
        RV_PUSH_SEC(RV_TAG_SUMM, rv_b_append_summ(b, &secs[nsec].body));
#undef RV_PUSH_SEC

    dir_offset = RV_FILE_HEADER_SIZE;
    payload_off = dir_offset + nsec * RV_DIR_ENTRY_SIZE;
    for (i = 0; i < nsec; i++) {
        uint8_t e[RV_DIR_ENTRY_SIZE];
        uint32_t crc = rv_crc32(secs[i].body.data, secs[i].body.size);
        memset(e, 0, sizeof(e));
        rv_write_le32(e + 0, secs[i].tag);
        rv_write_le64(e + 8, payload_off);
        rv_write_le32(e + 16, (uint32_t)secs[i].body.size);
        rv_write_le32(e + 20, crc);
        if (rv_buf_append(&dir, e, sizeof(e)) != 0)
            goto fail;
        payload_off += secs[i].body.size;
    }

    memset(hdr, 0, sizeof(hdr));
    memset(&h, 0, sizeof(h));
    h.magic[0] = RV_MAGIC0;
    h.magic[1] = RV_MAGIC1;
    h.magic[2] = RV_MAGIC2;
    h.magic[3] = RV_MAGIC3;
    h.ver_major = b->ver_major;
    h.ver_minor = b->ver_minor;
    h.created_unix = b->created_unix;
    h.section_count = nsec;
    h.dir_offset = dir_offset;
    h.total_size = payload_off;
    h.feature_bits = b->feature_bits;
    hdr[0] = RV_MAGIC0;
    hdr[1] = RV_MAGIC1;
    hdr[2] = RV_MAGIC2;
    hdr[3] = RV_MAGIC3;
    rv_write_le16(hdr + 4, h.ver_major);
    rv_write_le16(hdr + 6, h.ver_minor);
    rv_write_le32(hdr + 8, h.flags);
    rv_write_le64(hdr + 16, h.created_unix);
    rv_write_le32(hdr + 24, h.section_count);
    rv_write_le32(hdr + 28, h.dir_offset);
    rv_write_le64(hdr + 32, h.total_size);
    rv_write_le32(hdr + 40, h.feature_bits);
    h.header_crc = rv_crc32(hdr, sizeof(hdr));
    rv_write_le32(hdr + 12, h.header_crc);

    if (rv_buf_init(out, payload_off) != 0)
        goto fail;
    if (rv_buf_append(out, hdr, sizeof(hdr)) != 0 ||
        rv_buf_append(out, dir.data, dir.size) != 0)
        goto fail;
    for (i = 0; i < nsec; i++) {
        if (rv_buf_append(out, secs[i].body.data, secs[i].body.size) != 0)
            goto fail;
    }

    for (i = 0; i < nsec; i++)
        rv_buf_release(&secs[i].body);
    rv_buf_release(&dir);
    return 0;

fail:
    for (i = 0; i < 12; i++)
        rv_buf_release(&secs[i].body);
    rv_buf_release(&dir);
    rv_buf_release(out);
    return -1;
}

int rv_builder_write_file(rv_builder *b, const char *path)
{
    rv_buf out;
    FILE *fp;
    if (rv_buf_init(&out, 1024) != 0)
        return -1;
    if (rv_builder_serialize(b, &out) != 0) {
        rv_buf_release(&out);
        return -1;
    }
    fp = fopen(path, "wb");
    if (!fp) {
        rv_buf_release(&out);
        return -1;
    }
    if (fwrite(out.data, 1, out.size, fp) != out.size) {
        fclose(fp);
        rv_buf_release(&out);
        return -1;
    }
    fclose(fp);
    rv_buf_release(&out);
    return 0;
}

int rv_builder_demo_span(rv_builder *b, const char *site, const char *far,
                         float length_km, uint32_t sample_count)
{
    uint32_t site_id = 0, far_id = 0, model_id = 0, serial_id = 0, label_id = 0;
    rv_builder_route route;
    rv_builder_inst inst;
    rv_builder_calib calib;
    rv_builder_window win;
    rv_builder_marker mark;
    rv_builder_link link;
    int16_t *samples;
    uint32_t i;

    if (!b || !site || !far || sample_count == 0 || sample_count > 200000)
        return -1;
    if (rv_builder_add_name(b, site, &site_id) != 0 ||
        rv_builder_add_name(b, far, &far_id) != 0 ||
        rv_builder_add_name(b, "OTDR-X200", &model_id) != 0 ||
        rv_builder_add_name(b, "SN-10042", &serial_id) != 0 ||
        rv_builder_add_name(b, "splice", &label_id) != 0)
        return -1;

    memset(&route, 0, sizeof(route));
    route.route_id = 1;
    route.site_name_id = site_id;
    route.far_name_id = far_id;
    route.length_km = length_km;
    route.fiber_count = 12;
    if (rv_builder_add_route(b, &route) != 0)
        return -1;

    memset(&inst, 0, sizeof(inst));
    inst.inst_id = 1;
    inst.model_name_id = model_id;
    inst.serial_name_id = serial_id;
    inst.wavelength_nm = 1550;
    inst.pulse_default_ns = 100;
    inst.dynamic_range_db = 38.0f;
    if (rv_builder_add_inst(b, &inst) != 0)
        return -1;

    memset(&calib, 0, sizeof(calib));
    calib.calib_id = 1;
    calib.inst_id = 1;
    calib.calibrated_unix = 1700000000ull;
    calib.refractive_index = 1.4681f;
    calib.backscatter_coef = -81.0f;
    calib.splice_loss_db = 0.02f;
    if (rv_builder_add_calib(b, &calib) != 0)
        return -1;

    memset(&win, 0, sizeof(win));
    win.window_id = 1;
    win.route_id = 1;
    win.inst_id = 1;
    win.calib_id = 1;
    win.wave_block_id = 1;
    win.pulse_ns = 100.0f;
    win.range_km = length_km;
    win.resolution_m = 0.5f;
    win.acquired_unix = 1700001000ull;
    if (rv_builder_add_window(b, &win) != 0)
        return -1;

    samples = (int16_t *)malloc((size_t)sample_count * sizeof(int16_t));
    if (!samples)
        return -1;
    for (i = 0; i < sample_count; i++) {
        int v = -2000 + (int)(i % 50) - (int)(i / 200);
        if (v < -32000)
            v = -32000;
        samples[i] = (int16_t)v;
    }
    if (rv_builder_add_wave_copy(b, 1, 1, samples, sample_count, 0.0f, 0.5f) != 0) {
        free(samples);
        return -1;
    }
    free(samples);

    memset(&mark, 0, sizeof(mark));
    mark.marker_id = 1;
    mark.window_id = 1;
    mark.label_name_id = label_id;
    mark.distance_m = length_km * 500.0f;
    mark.loss_db = 0.15f;
    mark.reflectance_db = -55.0f;
    mark.kind = 1;
    mark.severity = 20;
    if (rv_builder_add_marker(b, &mark) != 0)
        return -1;

    memset(&link, 0, sizeof(link));
    link.link_id = 1;
    link.from_kind = 1;
    link.from_id = 1;
    link.to_kind = 2;
    link.to_id = 1;
    if (rv_builder_add_link(b, &link) != 0)
        return -1;

    return 0;
}
