#include "rv_section.h"
#include "rv_crc.h"
#include "rv_endian.h"
#include "rv_log.h"

#include <string.h>

const char *rv_tag_name(uint32_t tag)
{
    switch (tag) {
    case RV_TAG_SNAM: return "SNAM";
    case RV_TAG_ROUT: return "ROUT";
    case RV_TAG_INST: return "INST";
    case RV_TAG_CLBR: return "CLBR";
    case RV_TAG_WIND: return "WIND";
    case RV_TAG_WAVE: return "WAVE";
    case RV_TAG_MARK: return "MARK";
    case RV_TAG_LINK: return "LINK";
    case RV_TAG_NOTE: return "NOTE";
    case RV_TAG_SUMM: return "SUMM";
    default:          return "????";
    }
}

static int rv_read_lp_string(const uint8_t *p, size_t avail,
                             const char **out, uint16_t *out_len, size_t *consumed)
{
    uint16_t n;
    if (avail < 2)
        return -1;
    n = rv_read_le16(p);
    if ((size_t)n + 2 > avail)
        return -1;
    *out = (const char *)(p + 2);
    *out_len = n;
    *consumed = 2 + (size_t)n;
    return 0;
}

int rv_decode_snam(rv_arena *a, rv_slice payload, rv_snam_table *out)
{
    const uint8_t *p;
    size_t rem;
    uint32_t count, i, buckets;
    rv_name_entry *ents;

    if (!a || !out || !payload.ptr)
        return -1;
    memset(out, 0, sizeof(*out));
    if (payload.len < 8)
        return -1;
    p = payload.ptr;
    rem = payload.len;
    if (rv_read_le32(p) != 0x314D414Eu) /* "NAM1" record magic */
        return -1;
    count = rv_read_le32(p + 4);
    p += 8;
    rem -= 8;
    if (count > 100000u)
        return -1;

    ents = (rv_name_entry *)rv_arena_calloc(a, count, sizeof(rv_name_entry), 8);
    if (!ents && count)
        return -1;

    for (i = 0; i < count; i++) {
        const char *raw;
        uint16_t len;
        size_t used;
        char *copy;
        if (rem < 4)
            return -1;
        ents[i].id = rv_read_le32(p);
        p += 4;
        rem -= 4;
        if (rv_read_lp_string(p, rem, &raw, &len, &used) != 0)
            return -1;
        /*
         * Intern into the arena so later index layers can hold stable
         * pointers into the name pool for the life of the parse epoch.
         */
        copy = rv_arena_strdup(a, raw, len);
        if (!copy)
            return -1;
        ents[i].str = copy;
        ents[i].len = len;
        ents[i].hash = rv_hash32(copy, len);
        p += used;
        rem -= used;
        /* align to 2 bytes */
        if (used & 1u) {
            if (rem == 0)
                return -1;
            p++;
            rem--;
        }
    }

    buckets = 1;
    while (buckets < count * 2u + 8u)
        buckets <<= 1;
    out->bucket = (uint32_t *)rv_arena_calloc(a, buckets, sizeof(uint32_t), 4);
    if (!out->bucket)
        return -1;
    out->bucket_mask = buckets - 1;
    for (i = 0; i < count; i++) {
        uint32_t slot = ents[i].hash & out->bucket_mask;
        uint32_t probes = 0;
        while (out->bucket[slot] != 0 && probes < buckets) {
            slot = (slot + 1) & out->bucket_mask;
            probes++;
        }
        out->bucket[slot] = i + 1;
    }
    out->entries = ents;
    out->count = count;
    return 0;
}

int rv_decode_rout(rv_arena *a, rv_slice payload, rv_rout_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_route_rec *items;
    const size_t rec = 24;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x31544F52u) /* "ROT1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 50000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_route_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].route_id = rv_read_le32(p + 0);
        items[i].site_name_id = rv_read_le32(p + 4);
        items[i].far_name_id = rv_read_le32(p + 8);
        items[i].length_km = rv_read_f32(p + 12);
        items[i].fiber_count = rv_read_le32(p + 16);
        items[i].flags = rv_read_le32(p + 20);
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_inst(rv_arena *a, rv_slice payload, rv_inst_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_inst_rec *items;
    const size_t rec = 24;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x3154534Eu) /* "NST1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 10000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_inst_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].inst_id = rv_read_le32(p + 0);
        items[i].model_name_id = rv_read_le32(p + 4);
        items[i].serial_name_id = rv_read_le32(p + 8);
        items[i].wavelength_nm = rv_read_le16(p + 12);
        items[i].pulse_default_ns = rv_read_le16(p + 14);
        items[i].dynamic_range_db = rv_read_f32(p + 16);
        items[i].flags = rv_read_le32(p + 20);
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_clbr(rv_arena *a, rv_slice payload, rv_clbr_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_calib_rec *items;
    const size_t rec = 32;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x31424C43u) /* "CLB1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 10000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_calib_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].calib_id = rv_read_le32(p + 0);
        items[i].inst_id = rv_read_le32(p + 4);
        items[i].calibrated_unix = rv_read_le64(p + 8);
        items[i].refractive_index = rv_read_f32(p + 16);
        items[i].backscatter_coef = rv_read_f32(p + 20);
        items[i].splice_loss_db = rv_read_f32(p + 24);
        items[i].flags = rv_read_le32(p + 28);
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_wind(rv_arena *a, rv_slice payload, rv_wind_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_window_rec *items;
    const size_t rec = 40;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x31444E49u) /* "IND1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 200000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_window_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].window_id = rv_read_le32(p + 0);
        items[i].route_id = rv_read_le32(p + 4);
        items[i].inst_id = rv_read_le32(p + 8);
        items[i].calib_id = rv_read_le32(p + 12);
        items[i].wave_block_id = rv_read_le32(p + 16);
        items[i].pulse_ns = rv_read_f32(p + 20);
        items[i].range_km = rv_read_f32(p + 24);
        items[i].resolution_m = rv_read_f32(p + 28);
        items[i].acquired_unix = rv_read_le64(p + 32);
        /* flags packed after — extend carefully for v2.3 */
        items[i].flags = 0;
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_wave(rv_arena *a, rv_slice payload, rv_wave_table *out)
{
    const uint8_t *p;
    size_t rem;
    uint32_t count, i;
    rv_wave_block *items;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    rem = payload.len;
    if (rv_read_le32(p) != 0x31455641u) /* "AVE1" */
        return -1;
    count = rv_read_le32(p + 4);
    p += 8;
    rem -= 8;
    if (count > 50000u)
        return -1;
    items = (rv_wave_block *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;

    for (i = 0; i < count; i++) {
        uint32_t sc;
        size_t bytes;
        if (rem < 24)
            return -1;
        items[i].block_id = rv_read_le32(p + 0);
        items[i].window_id = rv_read_le32(p + 4);
        sc = rv_read_le32(p + 8);
        items[i].sample_count = sc;
        items[i].start_m = rv_read_f32(p + 12);
        items[i].step_m = rv_read_f32(p + 16);
        items[i].flags = rv_read_le16(p + 20);
        items[i].payload_crc = rv_read_le16(p + 22); /* truncated crc16 store */
        p += 24;
        rem -= 24;
        if (sc > 2000000u)
            return -1;
        bytes = (size_t)sc * sizeof(int16_t);
        if (bytes > rem)
            return -1;
        /* Borrow into package backing — valid while reader backing lives. */
        items[i].samples = (const int16_t *)(const void *)p;
        p += bytes;
        rem -= bytes;
        if (bytes & 1u) {
            if (rem == 0 && i + 1 < count)
                return -1;
            if (rem) {
                p++;
                rem--;
            }
        }
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_mark(rv_arena *a, rv_slice payload, rv_mark_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_marker_rec *items;
    const size_t rec = 32;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x314B524Du) /* "MRK1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 500000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_marker_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].marker_id = rv_read_le32(p + 0);
        items[i].window_id = rv_read_le32(p + 4);
        items[i].label_name_id = rv_read_le32(p + 8);
        items[i].distance_m = rv_read_f32(p + 12);
        items[i].loss_db = rv_read_f32(p + 16);
        items[i].reflectance_db = rv_read_f32(p + 20);
        items[i].kind = rv_read_le16(p + 24);
        items[i].severity = rv_read_le16(p + 26);
        items[i].flags = rv_read_le32(p + 28);
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_link(rv_arena *a, rv_slice payload, rv_link_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_link_rec *items;
    const size_t rec = 24;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x314B4E4Cu) /* "LNK1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 200000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_link_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].link_id = rv_read_le32(p + 0);
        items[i].from_kind = rv_read_le32(p + 4);
        items[i].from_id = rv_read_le32(p + 8);
        items[i].to_kind = rv_read_le32(p + 12);
        items[i].to_id = rv_read_le32(p + 16);
        items[i].flags = rv_read_le32(p + 20);
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_note(rv_arena *a, rv_slice payload, rv_note_table *out)
{
    const uint8_t *p;
    uint32_t count, i;
    rv_note_rec *items;
    const size_t rec = 20;

    if (!a || !out || payload.len < 8)
        return -1;
    memset(out, 0, sizeof(*out));
    p = payload.ptr;
    if (rv_read_le32(p) != 0x3145544Eu) /* "NTE1" */
        return -1;
    count = rv_read_le32(p + 4);
    if (count > 50000u || payload.len < 8 + count * rec)
        return -1;
    items = (rv_note_rec *)rv_arena_calloc(a, count, sizeof(*items), 8);
    if (!items && count)
        return -1;
    p += 8;
    for (i = 0; i < count; i++) {
        items[i].note_id = rv_read_le32(p + 0);
        items[i].target_kind = rv_read_le32(p + 4);
        items[i].target_id = rv_read_le32(p + 8);
        items[i].text_name_id = rv_read_le32(p + 12);
        items[i].flags = rv_read_le32(p + 16);
        p += rec;
    }
    out->items = items;
    out->count = count;
    return 0;
}

int rv_decode_summ(rv_slice payload, rv_summ_rec *out)
{
    const uint8_t *p;
    if (!out || payload.len < 40)
        return -1;
    p = payload.ptr;
    if (rv_read_le32(p) != 0x314D4D55u) /* "UMM1" */
        return -1;
    out->route_count = rv_read_le32(p + 4);
    out->window_count = rv_read_le32(p + 8);
    out->marker_count = rv_read_le32(p + 12);
    out->wave_count = rv_read_le32(p + 16);
    out->name_count = rv_read_le32(p + 20);
    out->package_crc = rv_read_le32(p + 24);
    out->payload_bytes = rv_read_le64(p + 28);
    out->flags = rv_read_le32(p + 36);
    return 0;
}
