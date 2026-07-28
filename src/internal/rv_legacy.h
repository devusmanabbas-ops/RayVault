#ifndef RV_LEGACY_H
#define RV_LEGACY_H

#include "rv_arena.h"
#include "rv_diag.h"
#include "rv_parser.h"
#include "rv_reader.h"

/*
 * Legacy v1 packages used a flatter section layout and stored sample
 * payloads inline with window records. Migration rebuilds v2 tables
 * into the arena and may replace the reader backing buffer.
 */

int rv_legacy_is_v1(const rv_reader *r);
int rv_legacy_migrate(rv_reader *r, rv_parsed *p, rv_arena *a, rv_diag *diag);

/* Deprecated wrappers kept for older call sites. */
int rv_legacy_open_compat(rv_reader *r, const uint8_t *data, size_t size);

#endif /* RV_LEGACY_H */
