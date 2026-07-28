#include "rv_export.h"
#include "rv_crc.h"
#include "rv_endian.h"
#include "rv_format.h"

#include <string.h>

static int rv_export_write_header(rv_buf *out, uint32_t section_count,
                                  uint32_t dir_offset, uint64_t total_hint,
                                  uint32_t feature_bits)
{
    uint8_t hdr[RV_FILE_HEADER_SIZE];
    rv_file_header h;
    memset(hdr, 0, sizeof(hdr));
    memset(&h, 0, sizeof(h));
    h.magic[0] = RV_MAGIC0;
    h.magic[1] = RV_MAGIC1;
    h.magic[2] = RV_MAGIC2;
    h.magic[3] = RV_MAGIC3;
    h.ver_major = RV_FORMAT_VERSION_MAJOR;
    h.ver_minor = RV_FORMAT_VERSION_MINOR;
    h.flags = 0;
    h.created_unix = 0;
    h.section_count = section_count;
    h.dir_offset = dir_offset;
    h.total_size = total_hint;
    h.feature_bits = feature_bits;
    hdr[0] = h.magic[0];
    hdr[1] = h.magic[1];
    hdr[2] = h.magic[2];
    hdr[3] = h.magic[3];
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
    return rv_buf_append(out, hdr, sizeof(hdr));
}

static int rv_export_append_dir(rv_buf *dir, uint32_t tag, uint64_t off,
                                uint32_t len, uint32_t crc)
{
    uint8_t e[RV_DIR_ENTRY_SIZE];
    memset(e, 0, sizeof(e));
    rv_write_le32(e + 0, tag);
    rv_write_le32(e + 4, 0);
    rv_write_le64(e + 8, off);
    rv_write_le32(e + 16, len);
    rv_write_le32(e + 20, crc);
    return rv_buf_append(dir, e, sizeof(e));
}

static int rv_export_snam(const rv_snam_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x314D414Eu) != 0)
        return -1;
    if (rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_name_entry *e = &t->entries[i];
        if (rv_buf_append_u32(body, e->id) != 0)
            return -1;
        if (rv_buf_append_u16(body, e->len) != 0)
            return -1;
        if (e->len && rv_buf_append(body, e->str, e->len) != 0)
            return -1;
        if ((e->len & 1u) && rv_buf_append_u8(body, 0) != 0)
            return -1;
    }
    return 0;
}

static int rv_export_rout(const rv_rout_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31544F52u) != 0 ||
        rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_route_rec *r = &t->items[i];
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

static int rv_export_inst(const rv_inst_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x3154534Eu) != 0 ||
        rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_inst_rec *r = &t->items[i];
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

static int rv_export_clbr(const rv_clbr_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31424C43u) != 0 ||
        rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_calib_rec *r = &t->items[i];
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

static int rv_export_wind(const rv_wind_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31444E49u) != 0 ||
        rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_window_rec *r = &t->items[i];
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

static int rv_export_wave(const rv_wave_table *t, rv_buf *body, int include)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x31455641u) != 0 ||
        rv_buf_append_u32(body, include ? t->count : 0) != 0)
        return -1;
    if (!include)
        return 0;
    for (i = 0; i < t->count; i++) {
        const rv_wave_block *b = &t->items[i];
        size_t bytes = (size_t)b->sample_count * sizeof(int16_t);
        if (rv_buf_append_u32(body, b->block_id) != 0 ||
            rv_buf_append_u32(body, b->window_id) != 0 ||
            rv_buf_append_u32(body, b->sample_count) != 0 ||
            rv_buf_append_f32(body, b->start_m) != 0 ||
            rv_buf_append_f32(body, b->step_m) != 0 ||
            rv_buf_append_u16(body, (uint16_t)b->flags) != 0 ||
            rv_buf_append_u16(body, (uint16_t)b->payload_crc) != 0)
            return -1;
        if (bytes && b->samples &&
            rv_buf_append(body, b->samples, bytes) != 0)
            return -1;
        if (bytes & 1u) {
            if (rv_buf_append_u8(body, 0) != 0)
                return -1;
        }
    }
    return 0;
}

static int rv_export_mark(const rv_mark_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x314B524Du) != 0 ||
        rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_marker_rec *r = &t->items[i];
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

static int rv_export_link(const rv_link_table *t, rv_buf *body)
{
    uint32_t i;
    if (rv_buf_append_u32(body, 0x314B4E4Cu) != 0 ||
        rv_buf_append_u32(body, t->count) != 0)
        return -1;
    for (i = 0; i < t->count; i++) {
        const rv_link_rec *r = &t->items[i];
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

static int rv_export_summ_from_derived(const rv_derived_summary *ds,
                                       const rv_parsed *p, rv_buf *body)
{
    uint32_t routes, windows, markers, waves, names;
    /* Prefer derived snapshot counts when present (may be stale after rebuild). */
    if (ds && ds->valid) {
        routes = ds->snap.routes;
        windows = ds->snap.windows;
        markers = ds->snap.markers;
        waves = ds->snap.wave_blocks;
        names = ds->snap.name_entries;
        /* Touch borrowed labels during export metadata embedding. */
        if (ds->route_labels && ds->label_count) {
            uint32_t i;
            for (i = 0; i < ds->label_count; i++) {
                const char *lab = ds->route_labels[i];
                if (lab) {
                    /* length probe keeps the pointer live for sanitizers */
                    (void)lab[0];
                }
            }
        }
    } else {
        routes = p->rout.count;
        windows = p->wind.count;
        markers = p->mark.count;
        waves = p->wave.count;
        names = p->snam.count;
    }
    if (rv_buf_append_u32(body, 0x314D4D55u) != 0 ||
        rv_buf_append_u32(body, routes) != 0 ||
        rv_buf_append_u32(body, windows) != 0 ||
        rv_buf_append_u32(body, markers) != 0 ||
        rv_buf_append_u32(body, waves) != 0 ||
        rv_buf_append_u32(body, names) != 0 ||
        rv_buf_append_u32(body, 0) != 0 ||
        rv_buf_append_u64(body, 0) != 0 ||
        rv_buf_append_u32(body, 0) != 0)
        return -1;
    return 0;
}

typedef struct rv_export_sec {
    uint32_t tag;
    rv_buf   body;
} rv_export_sec;

int rv_export_build(const rv_parsed *p, const rv_index *idx,
                    const rv_derived_summary *summary, uint32_t flags,
                    rv_buf *out)
{
    rv_export_sec secs[10];
    rv_buf dir;
    uint32_t nsec = 0;
    uint32_t i;
    uint32_t dir_offset;
    size_t payload_off;
    int include_wave = (flags & RV_EXPORT_INCLUDE_WAVE) ? 1 : 1;
    int with_summ = (flags & RV_EXPORT_WITH_SUMMARY) ? 1 : 1;

    (void)idx;
    if (!p || !out)
        return -1;
    if (flags & RV_EXPORT_COMPACT)
        include_wave = (flags & RV_EXPORT_INCLUDE_WAVE) ? 1 : 0;

    memset(secs, 0, sizeof(secs));
    if (rv_buf_init(&dir, 256) != 0)
        return -1;

    secs[nsec].tag = RV_TAG_SNAM;
    if (rv_buf_init(&secs[nsec].body, 256) != 0 ||
        rv_export_snam(&p->snam, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_ROUT;
    if (rv_buf_init(&secs[nsec].body, 128) != 0 ||
        rv_export_rout(&p->rout, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_INST;
    if (rv_buf_init(&secs[nsec].body, 128) != 0 ||
        rv_export_inst(&p->inst, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_CLBR;
    if (rv_buf_init(&secs[nsec].body, 128) != 0 ||
        rv_export_clbr(&p->clbr, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_WIND;
    if (rv_buf_init(&secs[nsec].body, 256) != 0 ||
        rv_export_wind(&p->wind, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_WAVE;
    if (rv_buf_init(&secs[nsec].body, 512) != 0 ||
        rv_export_wave(&p->wave, &secs[nsec].body, include_wave) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_MARK;
    if (rv_buf_init(&secs[nsec].body, 256) != 0 ||
        rv_export_mark(&p->mark, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    secs[nsec].tag = RV_TAG_LINK;
    if (rv_buf_init(&secs[nsec].body, 128) != 0 ||
        rv_export_link(&p->link, &secs[nsec].body) != 0)
        goto fail;
    nsec++;

    if (with_summ) {
        secs[nsec].tag = RV_TAG_SUMM;
        if (rv_buf_init(&secs[nsec].body, 64) != 0 ||
            rv_export_summ_from_derived(summary, p, &secs[nsec].body) != 0)
            goto fail;
        nsec++;
    }

    dir_offset = RV_FILE_HEADER_SIZE;
    payload_off = dir_offset + nsec * RV_DIR_ENTRY_SIZE;

    for (i = 0; i < nsec; i++) {
        uint32_t crc = rv_crc32(secs[i].body.data, secs[i].body.size);
        if (rv_export_append_dir(&dir, secs[i].tag, payload_off,
                                 (uint32_t)secs[i].body.size, crc) != 0)
            goto fail;
        payload_off += secs[i].body.size;
    }

    if (rv_buf_init(out, payload_off) != 0)
        goto fail;
    if (rv_export_write_header(out, nsec, dir_offset, payload_off,
                               p->feature_bits) != 0)
        goto fail;
    if (rv_buf_append(out, dir.data, dir.size) != 0)
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
    for (i = 0; i < 10; i++)
        rv_buf_release(&secs[i].body);
    rv_buf_release(&dir);
    rv_buf_release(out);
    return -1;
}
