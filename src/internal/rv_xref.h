#ifndef RV_XREF_H
#define RV_XREF_H

#include "rv_diag.h"
#include "rv_parser.h"
#include "rv_index.h"

/*
 * Cross-section reference resolver. Ensures WIND->WAVE, WIND->ROUT,
 * MARK->WIND, CLBR->INST, and LINK endpoints agree.
 */

typedef struct rv_xref_report {
    uint32_t checked;
    uint32_t failures;
    uint32_t repaired;
} rv_xref_report;

int rv_xref_validate(const rv_parsed *p, const rv_index *idx,
                     rv_diag *diag, rv_xref_report *rep);
int rv_xref_resolve_wave_for_window(const rv_parsed *p, const rv_index *idx,
                                    uint32_t window_id, uint32_t *wave_pos);
int rv_xref_resolve_calib_for_inst(const rv_parsed *p, uint32_t inst_id,
                                   uint32_t *calib_pos);

#endif /* RV_XREF_H */
