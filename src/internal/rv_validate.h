#ifndef RV_VALIDATE_H
#define RV_VALIDATE_H

#include "rv_diag.h"
#include "rv_index.h"
#include "rv_parser.h"

typedef struct rv_validate_options {
    int check_crc;
    int check_xref;
    int check_summ;
    int check_ranges;
    int check_names;
} rv_validate_options;

void rv_validate_options_default(rv_validate_options *o);
int  rv_validate_package(const rv_reader *r, const rv_parsed *p,
                         const rv_index *idx, const rv_validate_options *o,
                         rv_diag *diag);

int rv_validate_header_consistency(const rv_reader *r, rv_diag *diag);
int rv_validate_summ_counts(const rv_parsed *p, rv_diag *diag);
int rv_validate_physical_ranges(const rv_parsed *p, rv_diag *diag);

#endif /* RV_VALIDATE_H */
