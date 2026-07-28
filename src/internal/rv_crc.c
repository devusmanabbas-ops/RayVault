#include "rv_crc.h"

#include <string.h>

static uint32_t rv_crc_table[256];
static int rv_crc_ready;

static void rv_crc_init_table(void)
{
    uint32_t i, j, c;
    for (i = 0; i < 256; i++) {
        c = i;
        for (j = 0; j < 8; j++)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        rv_crc_table[i] = c;
    }
    rv_crc_ready = 1;
}

uint32_t rv_crc32_update(uint32_t crc, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    if (!rv_crc_ready)
        rv_crc_init_table();
    crc = ~crc;
    while (len--) {
        crc = rv_crc_table[(crc ^ *p++) & 0xFFu] ^ (crc >> 8);
    }
    return ~crc;
}

uint32_t rv_crc32(const void *data, size_t len)
{
    return rv_crc32_update(0, data, len);
}

uint64_t rv_fnv1a64(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint64_t h = 14695981039346656037ull;
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

uint32_t rv_hash32(const void *data, size_t len)
{
    /* Murmur-inspired short mix for dictionary buckets. */
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = 0x811C9DC5u;
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x01000193u;
    }
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}
