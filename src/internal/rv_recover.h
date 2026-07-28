#ifndef RV_RECOVER_H
#define RV_RECOVER_H

#include "rv_diag.h"
#include "rv_parser.h"
#include "rv_reader.h"

typedef struct rv_repair_options {
    int rebuild_summ;
    int drop_bad_links;
    int clamp_ranges;
    int synthesize_missing_names;
} rv_repair_options;

void rv_repair_options_default(rv_repair_options *o);
int  rv_repair_package(rv_reader *r, rv_parsed *p, rv_arena *a,
                       const rv_repair_options *o, rv_diag *diag);

#endif /* RV_RECOVER_H */
