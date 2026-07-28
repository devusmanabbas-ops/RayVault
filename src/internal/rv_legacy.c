#include "rv_legacy.h"
#include "rv_buf.h"
#include "rv_endian.h"
#include "rv_export.h"
#include "rv_log.h"
#include "rv_format.h"
#include "rv_crc.h"

#include <stdlib.h>
#include <string.h>

int rv_legacy_is_v1(const rv_reader *r)
{
    if (!r)
        return 0;
    return r->header.ver_major == RV_FORMAT_VERSION_LEGACY_MAJOR;
}

int rv_legacy_open_compat(rv_reader *r, const uint8_t *data, size_t size)
{
    return rv_reader_open_memory(r, data, size, RV_OPEN_ALLOW_LEGACY, 1);
}

static int rv_legacy_reload_directory(rv_reader *r)
{
    size_t off;
    uint32_t i;
    const uint8_t *base;

    if (rv_parse_file_header(r->backing.data, r->backing.size, &r->header) != 0)
        return -1;
    r->dir_count = r->header.section_count;
    off = r->header.dir_offset;
    base = r->backing.data;
    for (i = 0; i < r->dir_count; i++) {
        if (off + RV_DIR_ENTRY_SIZE > r->backing.size)
            return -1;
        if (rv_parse_dir_entry(base + off, RV_DIR_ENTRY_SIZE, &r->dir[i]) != 0)
            return -1;
        off += RV_DIR_ENTRY_SIZE;
    }
    return 0;
}

/*
 * Migration rebuilds a v2 image and swaps the reader backing buffer.
 * Existing parsed WAVE sample pointers still refer to the previous
 * backing until the caller re-runs the parser. Session caches that
 * borrowed those samples similarly retain the old addresses.
 */
int rv_legacy_migrate(rv_reader *r, rv_parsed *p, rv_arena *a, rv_diag *diag)
{
    rv_buf exported;
    rv_index dummy;
    uint8_t *owned;
    size_t sz;
    int from_file;
    char path[512];
    uint32_t flags;

    if (!r || !p || !a)
        return -1;
    if (!rv_legacy_is_v1(r) && !(r->open_flags & RV_OPEN_ALLOW_LEGACY)) {
        if (diag)
            rv_diag_add(diag, RV_ERR_LEGACY, "not a legacy package");
        return -1;
    }

    memset(&dummy, 0, sizeof(dummy));
    if (rv_buf_init(&exported, 1024) != 0)
        return -1;

    if (rv_export_build(p, &dummy, NULL,
                        RV_EXPORT_INCLUDE_WAVE | RV_EXPORT_WITH_SUMMARY,
                        &exported) != 0) {
        rv_buf_release(&exported);
        if (diag)
            rv_diag_add(diag, RV_ERR_LEGACY, "export during migrate failed");
        return -1;
    }

    sz = exported.size;
    owned = exported.data;
    exported.data = NULL;
    exported.owned = 0;
    rv_buf_release(&exported);

    from_file = r->from_file;
    flags = r->open_flags;
    memcpy(path, r->path, sizeof(path));

    /* Drop previous backing; wave samples in `p` become dangling. */
    rv_buf_release(&r->backing);
    if (rv_buf_wrap(&r->backing, owned, sz, 1) != 0) {
        free(owned);
        return -1;
    }
    r->from_file = from_file;
    r->open_flags = flags | RV_OPEN_ALLOW_LEGACY;
    memcpy(r->path, path, sizeof(r->path));

    if (rv_legacy_reload_directory(r) != 0) {
        if (diag)
            rv_diag_add(diag, RV_ERR_LEGACY, "directory reload failed");
        return -1;
    }

    p->feature_bits |= 0x100u;
    r->header.ver_major = RV_FORMAT_VERSION_MAJOR;
    r->header.ver_minor = RV_FORMAT_VERSION_MINOR;

    if (diag)
        rv_diag_add(diag, RV_OK, "legacy migration applied");
    (void)a;
    return 0;
}
