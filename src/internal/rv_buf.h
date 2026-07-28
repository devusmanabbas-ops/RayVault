#ifndef RV_BUF_H
#define RV_BUF_H

#include <stddef.h>
#include <stdint.h>

typedef struct rv_buf {
    uint8_t *data;
    size_t   size;
    size_t   cap;
    int      owned;
} rv_buf;

int  rv_buf_init(rv_buf *b, size_t cap);
int  rv_buf_wrap(rv_buf *b, uint8_t *data, size_t size, int take_ownership);
int  rv_buf_reserve(rv_buf *b, size_t need);
int  rv_buf_append(rv_buf *b, const void *src, size_t n);
int  rv_buf_append_u8(rv_buf *b, uint8_t v);
int  rv_buf_append_u16(rv_buf *b, uint16_t v);
int  rv_buf_append_u32(rv_buf *b, uint32_t v);
int  rv_buf_append_u64(rv_buf *b, uint64_t v);
int  rv_buf_append_f32(rv_buf *b, float v);
void rv_buf_clear(rv_buf *b);
void rv_buf_release(rv_buf *b);

const uint8_t *rv_buf_data(const rv_buf *b);
size_t         rv_buf_size(const rv_buf *b);

typedef struct rv_slice {
    const uint8_t *ptr;
    size_t         len;
} rv_slice;

int rv_slice_sub(rv_slice base, size_t off, size_t len, rv_slice *out);

#endif /* RV_BUF_H */
