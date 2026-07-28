#ifndef RV_ENDIAN_H
#define RV_ENDIAN_H

#include <stdint.h>
#include <string.h>

static inline uint16_t rv_read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t rv_read_le32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static inline uint64_t rv_read_le64(const uint8_t *p)
{
    uint64_t lo = rv_read_le32(p);
    uint64_t hi = rv_read_le32(p + 4);
    return lo | (hi << 32);
}

static inline float rv_read_f32(const uint8_t *p)
{
    float f;
    uint32_t u = rv_read_le32(p);
    memcpy(&f, &u, sizeof(f));
    return f;
}

static inline void rv_write_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}

static inline void rv_write_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static inline void rv_write_le64(uint8_t *p, uint64_t v)
{
    rv_write_le32(p, (uint32_t)v);
    rv_write_le32(p + 4, (uint32_t)(v >> 32));
}

static inline void rv_write_f32(uint8_t *p, float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    rv_write_le32(p, u);
}

#endif /* RV_ENDIAN_H */
