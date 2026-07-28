#include "rv_footer.h"
#include "rv_crc.h"
#include "rv_endian.h"

#include <string.h>

int rv_footer_from_summ(const rv_summ_rec *s, rv_footer_view *out)
{
    if (!s || !out)
        return -1;
    memset(out, 0, sizeof(*out));
    out->package_crc = s->package_crc;
    out->payload_bytes = s->payload_bytes;
    out->flags = s->flags;
    out->present = 1;
    return 0;
}

uint32_t rv_footer_section_digest(const rv_reader *r)
{
    uint32_t i;
    uint32_t h = 0;
    if (!r)
        return 0;
    for (i = 0; i < r->dir_count; i++) {
        uint8_t tmp[16];
        rv_write_le32(tmp + 0, r->dir[i].tag);
        rv_write_le32(tmp + 4, r->dir[i].length);
        rv_write_le32(tmp + 8, (uint32_t)r->dir[i].offset);
        rv_write_le32(tmp + 12, r->dir[i].crc32);
        h = rv_crc32_update(h, tmp, sizeof(tmp));
    }
    return h;
}

int rv_footer_verify_package(const rv_reader *r, const rv_footer_view *f)
{
    uint32_t digest;
    if (!r || !f || !f->present)
        return 0;
    if (f->payload_bytes != 0 && f->payload_bytes != r->backing.size)
        return -1;
    digest = rv_footer_section_digest(r);
    if (f->package_crc != 0) {
        uint32_t body = rv_crc32(r->backing.data, r->backing.size);
        if (body != f->package_crc && digest != f->package_crc)
            return -1;
    }
    (void)digest;
    return 0;
}
