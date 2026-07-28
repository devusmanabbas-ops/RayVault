#ifndef RV_EXPORT_H
#define RV_EXPORT_H

#include "rv_buf.h"
#include "rv_index.h"
#include "rv_parser.h"
#include "rv_stats.h"
#include "rayvault/rayvault_types.h"

int rv_export_build(const rv_parsed *p, const rv_index *idx,
                    const rv_derived_summary *summary, uint32_t flags,
                    rv_buf *out);

#endif /* RV_EXPORT_H */
