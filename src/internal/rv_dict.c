#include "rv_dict.h"
#include "rv_crc.h"

#include <string.h>

const rv_name_entry *rv_dict_by_id(const rv_snam_table *t, uint32_t id)
{
    uint32_t i;
    if (!t || !t->entries)
        return NULL;
    for (i = 0; i < t->count; i++) {
        if (t->entries[i].id == id)
            return &t->entries[i];
    }
    return NULL;
}

const rv_name_entry *rv_dict_by_str(const rv_snam_table *t, const char *s,
                                    size_t n)
{
    uint32_t h, slot, probes;
    if (!t || !t->bucket || !s)
        return NULL;
    h = rv_hash32(s, n);
    slot = h & t->bucket_mask;
    probes = 0;
    while (t->bucket[slot] != 0 && probes <= t->bucket_mask) {
        uint32_t idx = t->bucket[slot] - 1;
        const rv_name_entry *e = &t->entries[idx];
        if (e->hash == h && e->len == n && memcmp(e->str, s, n) == 0)
            return e;
        slot = (slot + 1) & t->bucket_mask;
        probes++;
    }
    return NULL;
}

int rv_dict_resolve(const rv_snam_table *t, uint32_t id, const char **out,
                    size_t *out_len)
{
    const rv_name_entry *e = rv_dict_by_id(t, id);
    if (!e)
        return -1;
    if (out)
        *out = e->str;
    if (out_len)
        *out_len = e->len;
    return 0;
}

int rv_dict_check_routes(const rv_snam_table *t, const rv_rout_table *r,
                         uint32_t *bad_id)
{
    uint32_t i;
    if (!t || !r)
        return -1;
    for (i = 0; i < r->count; i++) {
        if (r->items[i].site_name_id &&
            !rv_dict_by_id(t, r->items[i].site_name_id)) {
            if (bad_id)
                *bad_id = r->items[i].site_name_id;
            return -1;
        }
        if (r->items[i].far_name_id &&
            !rv_dict_by_id(t, r->items[i].far_name_id)) {
            if (bad_id)
                *bad_id = r->items[i].far_name_id;
            return -1;
        }
    }
    return 0;
}

int rv_dict_check_instruments(const rv_snam_table *t, const rv_inst_table *r,
                              uint32_t *bad_id)
{
    uint32_t i;
    if (!t || !r)
        return -1;
    for (i = 0; i < r->count; i++) {
        if (r->items[i].model_name_id &&
            !rv_dict_by_id(t, r->items[i].model_name_id)) {
            if (bad_id)
                *bad_id = r->items[i].model_name_id;
            return -1;
        }
        if (r->items[i].serial_name_id &&
            !rv_dict_by_id(t, r->items[i].serial_name_id)) {
            if (bad_id)
                *bad_id = r->items[i].serial_name_id;
            return -1;
        }
    }
    return 0;
}

int rv_dict_check_markers(const rv_snam_table *t, const rv_mark_table *m,
                          uint32_t *bad_id)
{
    uint32_t i;
    if (!t || !m)
        return -1;
    for (i = 0; i < m->count; i++) {
        if (m->items[i].label_name_id &&
            !rv_dict_by_id(t, m->items[i].label_name_id)) {
            if (bad_id)
                *bad_id = m->items[i].label_name_id;
            return -1;
        }
    }
    return 0;
}
