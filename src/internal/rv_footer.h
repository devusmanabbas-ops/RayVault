#ifndef RV_FOOTER_H
#define RV_FOOTER_H

#include "rv_format.h"
#include "rv_reader.h"

#include <stdint.h>

typedef struct rv_footer_view {
    uint32_t package_crc;
    uint32_t section_digest;
    uint64_t payload_bytes;
    uint32_t flags;
    int      present;
} rv_footer_view;

int rv_footer_from_summ(const rv_summ_rec *s, rv_footer_view *out);
int rv_footer_verify_package(const rv_reader *r, const rv_footer_view *f);
uint32_t rv_footer_section_digest(const rv_reader *r);

#endif /* RV_FOOTER_H */
