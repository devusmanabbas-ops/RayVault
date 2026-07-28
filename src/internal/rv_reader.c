#include "rv_reader.h"
#include "rv_crc.h"
#include "rv_endian.h"
#include "rv_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int rv_reader_load_directory(rv_reader *r)
{
    size_t off;
    uint32_t i;
    const uint8_t *base;

    if (rv_parse_file_header(r->backing.data, r->backing.size, &r->header) != 0) {
        rv_diag_add(&r->diag, RV_ERR_FORMAT, "invalid file header");
        return -1;
    }
    if (r->header.ver_major > RV_FORMAT_VERSION_MAJOR) {
        rv_diag_add(&r->diag, RV_ERR_VERSION, "unsupported major version %u",
                    r->header.ver_major);
        return -1;
    }
    if (r->header.ver_major < RV_FORMAT_VERSION_LEGACY_MAJOR) {
        rv_diag_add(&r->diag, RV_ERR_VERSION, "version too old: %u",
                    r->header.ver_major);
        return -1;
    }

    r->dir_count = r->header.section_count;
    off = r->header.dir_offset;
    base = r->backing.data;
    for (i = 0; i < r->dir_count; i++) {
        if (off + RV_DIR_ENTRY_SIZE > r->backing.size) {
            rv_diag_add(&r->diag, RV_ERR_TRUNCATED, "directory truncated at %u", i);
            return -1;
        }
        if (rv_parse_dir_entry(base + off, RV_DIR_ENTRY_SIZE, &r->dir[i]) != 0) {
            rv_diag_add(&r->diag, RV_ERR_SECTION, "bad directory entry %u", i);
            return -1;
        }
        if (r->dir[i].offset + r->dir[i].length > r->backing.size) {
            rv_diag_add(&r->diag, RV_ERR_OFFSET,
                        "section %u offset/length out of range", i);
            return -1;
        }
        off += RV_DIR_ENTRY_SIZE;
    }
    return 0;
}

int rv_reader_open_file(rv_reader *r, const char *path, uint32_t flags)
{
    FILE *fp;
    long sz;
    uint8_t *buf;

    if (!r || !path)
        return -1;
    memset(r, 0, sizeof(*r));
    rv_diag_clear(&r->diag);
    r->open_flags = flags;
    r->from_file = 1;
    strncpy(r->path, path, sizeof(r->path) - 1);

    fp = fopen(path, "rb");
    if (!fp) {
        rv_diag_add(&r->diag, RV_ERR_IO, "cannot open %s", path);
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    sz = ftell(fp);
    if (sz < (long)RV_FILE_HEADER_SIZE) {
        fclose(fp);
        rv_diag_add(&r->diag, RV_ERR_TRUNCATED, "file too small");
        return -1;
    }
    rewind(fp);
    buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) {
        fclose(fp);
        return -1;
    }
    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
        free(buf);
        fclose(fp);
        rv_diag_add(&r->diag, RV_ERR_IO, "short read");
        return -1;
    }
    fclose(fp);
    if (rv_buf_wrap(&r->backing, buf, (size_t)sz, 1) != 0) {
        free(buf);
        return -1;
    }
    return rv_reader_load_directory(r);
}

int rv_reader_open_memory(rv_reader *r, const uint8_t *data, size_t size,
                          uint32_t flags, int copy)
{
    uint8_t *buf;
    if (!r || !data || size < RV_FILE_HEADER_SIZE)
        return -1;
    memset(r, 0, sizeof(*r));
    rv_diag_clear(&r->diag);
    r->open_flags = flags;
    r->from_file = 0;
    if (copy) {
        buf = (uint8_t *)malloc(size);
        if (!buf)
            return -1;
        memcpy(buf, data, size);
        if (rv_buf_wrap(&r->backing, buf, size, 1) != 0) {
            free(buf);
            return -1;
        }
    } else {
        if (rv_buf_wrap(&r->backing, (uint8_t *)(uintptr_t)data, size, 0) != 0)
            return -1;
    }
    return rv_reader_load_directory(r);
}

int rv_reader_reload(rv_reader *r)
{
    char path[512];
    uint32_t flags;
    if (!r || !r->from_file || r->path[0] == '\0')
        return -1;
    strncpy(path, r->path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    flags = r->open_flags;
    rv_reader_close(r);
    return rv_reader_open_file(r, path, flags);
}

void rv_reader_close(rv_reader *r)
{
    if (!r)
        return;
    rv_buf_release(&r->backing);
    memset(r, 0, sizeof(*r));
}

const uint8_t *rv_reader_bytes(const rv_reader *r)
{
    return r ? r->backing.data : NULL;
}

size_t rv_reader_size(const rv_reader *r)
{
    return r ? r->backing.size : 0;
}

int rv_reader_find_section(const rv_reader *r, uint32_t tag, uint32_t nth,
                           rv_dir_entry *out)
{
    uint32_t i, seen = 0;
    if (!r || !out)
        return -1;
    for (i = 0; i < r->dir_count; i++) {
        if (r->dir[i].tag != tag)
            continue;
        if (seen == nth) {
            *out = r->dir[i];
            return 0;
        }
        seen++;
    }
    return -1;
}

int rv_reader_section_slice(const rv_reader *r, const rv_dir_entry *e,
                            rv_slice *out)
{
    rv_slice base;
    if (!r || !e || !out)
        return -1;
    base.ptr = r->backing.data;
    base.len = r->backing.size;
    return rv_slice_sub(base, (size_t)e->offset, e->length, out);
}

int rv_reader_verify_section_crc(const rv_reader *r, const rv_dir_entry *e)
{
    rv_slice s;
    uint32_t c;
    if (!r || !e)
        return -1;
    if (e->crc32 == 0)
        return 0; /* optional */
    if (rv_reader_section_slice(r, e, &s) != 0)
        return -1;
    c = rv_crc32(s.ptr, s.len);
    return c == e->crc32 ? 0 : -1;
}
