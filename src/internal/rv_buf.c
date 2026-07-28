#include "rv_buf.h"
#include "rv_endian.h"

#include <stdlib.h>
#include <string.h>

int rv_buf_init(rv_buf *b, size_t cap)
{
    if (!b)
        return -1;
    memset(b, 0, sizeof(*b));
    if (cap == 0)
        cap = 64;
    b->data = (uint8_t *)malloc(cap);
    if (!b->data)
        return -1;
    b->cap = cap;
    b->size = 0;
    b->owned = 1;
    return 0;
}

int rv_buf_wrap(rv_buf *b, uint8_t *data, size_t size, int take_ownership)
{
    if (!b)
        return -1;
    memset(b, 0, sizeof(*b));
    b->data = data;
    b->size = size;
    b->cap = size;
    b->owned = take_ownership ? 1 : 0;
    return 0;
}

int rv_buf_reserve(rv_buf *b, size_t need)
{
    size_t ncap;
    uint8_t *nd;

    if (!b)
        return -1;
    if (need <= b->cap)
        return 0;
    if (!b->owned)
        return -1;
    ncap = b->cap ? b->cap : 64;
    while (ncap < need) {
        if (ncap > ((size_t)-1) / 2)
            return -1;
        ncap *= 2;
    }
    nd = (uint8_t *)realloc(b->data, ncap);
    if (!nd)
        return -1;
    b->data = nd;
    b->cap = ncap;
    return 0;
}

int rv_buf_append(rv_buf *b, const void *src, size_t n)
{
    if (!b || (!src && n))
        return -1;
    if (n == 0)
        return 0;
    if (b->size > ((size_t)-1) - n)
        return -1;
    if (rv_buf_reserve(b, b->size + n) != 0)
        return -1;
    memcpy(b->data + b->size, src, n);
    b->size += n;
    return 0;
}

int rv_buf_append_u8(rv_buf *b, uint8_t v)
{
    return rv_buf_append(b, &v, 1);
}

int rv_buf_append_u16(rv_buf *b, uint16_t v)
{
    uint8_t tmp[2];
    rv_write_le16(tmp, v);
    return rv_buf_append(b, tmp, 2);
}

int rv_buf_append_u32(rv_buf *b, uint32_t v)
{
    uint8_t tmp[4];
    rv_write_le32(tmp, v);
    return rv_buf_append(b, tmp, 4);
}

int rv_buf_append_u64(rv_buf *b, uint64_t v)
{
    uint8_t tmp[8];
    rv_write_le64(tmp, v);
    return rv_buf_append(b, tmp, 8);
}

int rv_buf_append_f32(rv_buf *b, float v)
{
    uint8_t tmp[4];
    rv_write_f32(tmp, v);
    return rv_buf_append(b, tmp, 4);
}

void rv_buf_clear(rv_buf *b)
{
    if (b)
        b->size = 0;
}

void rv_buf_release(rv_buf *b)
{
    if (!b)
        return;
    if (b->owned && b->data)
        free(b->data);
    memset(b, 0, sizeof(*b));
}

const uint8_t *rv_buf_data(const rv_buf *b)
{
    return b ? b->data : NULL;
}

size_t rv_buf_size(const rv_buf *b)
{
    return b ? b->size : 0;
}

int rv_slice_sub(rv_slice base, size_t off, size_t len, rv_slice *out)
{
    if (!out || !base.ptr)
        return -1;
    if (off > base.len || len > base.len - off)
        return -1;
    out->ptr = base.ptr + off;
    out->len = len;
    return 0;
}
