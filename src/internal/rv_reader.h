#ifndef RV_READER_H
#define RV_READER_H

#include "rv_buf.h"
#include "rv_diag.h"
#include "rv_format.h"

#include <stddef.h>
#include <stdint.h>

typedef struct rv_reader {
    rv_buf         backing;
    rv_file_header header;
    rv_dir_entry   dir[RV_MAX_SECTIONS];
    uint32_t       dir_count;
    rv_diag        diag;
    int            from_file;
    char           path[512];
    uint32_t       open_flags;
} rv_reader;

int  rv_reader_open_file(rv_reader *r, const char *path, uint32_t flags);
int  rv_reader_open_memory(rv_reader *r, const uint8_t *data, size_t size,
                           uint32_t flags, int copy);
int  rv_reader_reload(rv_reader *r);
void rv_reader_close(rv_reader *r);

const uint8_t *rv_reader_bytes(const rv_reader *r);
size_t         rv_reader_size(const rv_reader *r);

int rv_reader_find_section(const rv_reader *r, uint32_t tag, uint32_t nth,
                           rv_dir_entry *out);
int rv_reader_section_slice(const rv_reader *r, const rv_dir_entry *e,
                            rv_slice *out);
int rv_reader_verify_section_crc(const rv_reader *r, const rv_dir_entry *e);

#endif /* RV_READER_H */
