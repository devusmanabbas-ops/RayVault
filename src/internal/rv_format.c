#include "rv_format.h"
#include "rv_endian.h"
#include "rv_crc.h"

#include <string.h>

uint32_t rv_header_crc_compute(const rv_file_header *h)
{
    uint8_t tmp[RV_FILE_HEADER_SIZE];
    memset(tmp, 0, sizeof(tmp));
    tmp[0] = h->magic[0];
    tmp[1] = h->magic[1];
    tmp[2] = h->magic[2];
    tmp[3] = h->magic[3];
    rv_write_le16(tmp + 4, h->ver_major);
    rv_write_le16(tmp + 6, h->ver_minor);
    rv_write_le32(tmp + 8, h->flags);
    /* bytes 12-15 are header_crc itself — zeroed for computation */
    rv_write_le64(tmp + 16, h->created_unix);
    rv_write_le32(tmp + 24, h->section_count);
    rv_write_le32(tmp + 28, h->dir_offset);
    rv_write_le64(tmp + 32, h->total_size);
    rv_write_le32(tmp + 40, h->feature_bits);
    rv_write_le32(tmp + 44, h->reserved0);
    rv_write_le64(tmp + 48, h->reserved1);
    return rv_crc32(tmp, sizeof(tmp));
}

int rv_parse_file_header(const uint8_t *data, size_t size, rv_file_header *out)
{
    uint32_t expect;
    if (!data || !out || size < RV_FILE_HEADER_SIZE)
        return -1;
    memset(out, 0, sizeof(*out));
    out->magic[0] = data[0];
    out->magic[1] = data[1];
    out->magic[2] = data[2];
    out->magic[3] = data[3];
    if (out->magic[0] != RV_MAGIC0 || out->magic[1] != RV_MAGIC1 ||
        out->magic[2] != RV_MAGIC2 || out->magic[3] != RV_MAGIC3)
        return -1;
    out->ver_major = rv_read_le16(data + 4);
    out->ver_minor = rv_read_le16(data + 6);
    out->flags = rv_read_le32(data + 8);
    out->header_crc = rv_read_le32(data + 12);
    out->created_unix = rv_read_le64(data + 16);
    out->section_count = rv_read_le32(data + 24);
    out->dir_offset = rv_read_le32(data + 28);
    out->total_size = rv_read_le64(data + 32);
    out->feature_bits = rv_read_le32(data + 40);
    out->reserved0 = rv_read_le32(data + 44);
    out->reserved1 = rv_read_le64(data + 48);

    if (out->section_count > RV_MAX_SECTIONS)
        return -1;
    if (out->dir_offset < RV_FILE_HEADER_SIZE)
        return -1;
    if ((uint64_t)out->dir_offset + (uint64_t)out->section_count * RV_DIR_ENTRY_SIZE > size)
        return -1;

    expect = rv_header_crc_compute(out);
    if (out->header_crc != 0 && out->header_crc != expect)
        return -2;
    return 0;
}

int rv_parse_dir_entry(const uint8_t *data, size_t size, rv_dir_entry *out)
{
    if (!data || !out || size < RV_DIR_ENTRY_SIZE)
        return -1;
    out->tag = rv_read_le32(data + 0);
    out->flags = rv_read_le32(data + 4);
    out->offset = rv_read_le64(data + 8);
    out->length = rv_read_le32(data + 16);
    out->crc32 = rv_read_le32(data + 20);
    return 0;
}
