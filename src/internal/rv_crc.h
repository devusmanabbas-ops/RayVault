#ifndef RV_CRC_H
#define RV_CRC_H

#include <stddef.h>
#include <stdint.h>

uint32_t rv_crc32(const void *data, size_t len);
uint32_t rv_crc32_update(uint32_t crc, const void *data, size_t len);
uint64_t rv_fnv1a64(const void *data, size_t len);
uint32_t rv_hash32(const void *data, size_t len);

#endif /* RV_CRC_H */
