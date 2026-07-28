#ifndef RV_PARSER_H
#define RV_PARSER_H

#include "rv_arena.h"
#include "rv_diag.h"
#include "rv_reader.h"
#include "rv_section.h"

typedef enum rv_parse_phase {
    RV_PHASE_INIT = 0,
    RV_PHASE_HEADER,
    RV_PHASE_DIR,
    RV_PHASE_SNAM,
    RV_PHASE_ROUT,
    RV_PHASE_INST,
    RV_PHASE_CLBR,
    RV_PHASE_WIND,
    RV_PHASE_WAVE,
    RV_PHASE_MARK,
    RV_PHASE_LINK,
    RV_PHASE_NOTE,
    RV_PHASE_SUMM,
    RV_PHASE_DONE,
    RV_PHASE_ERROR
} rv_parse_phase;

typedef struct rv_parsed {
    rv_snam_table snam;
    rv_rout_table rout;
    rv_inst_table inst;
    rv_clbr_table clbr;
    rv_wind_table wind;
    rv_wave_table wave;
    rv_mark_table mark;
    rv_link_table link;
    rv_note_table note;
    rv_summ_rec   summ;
    int           has_summ;
    rv_parse_phase phase;
    uint32_t      sections_seen;
    uint32_t      feature_bits;
} rv_parsed;

typedef struct rv_parser {
    rv_reader *reader;
    rv_arena  *arena;
    rv_diag   *diag;
    rv_parsed  result;
    uint32_t   flags;
    int        incremental;
    int        step_index;
} rv_parser;

void rv_parser_init(rv_parser *p, rv_reader *r, rv_arena *a, rv_diag *d,
                    uint32_t flags);
int  rv_parser_run(rv_parser *p);
int  rv_parser_step(rv_parser *p);
void rv_parser_reset(rv_parser *p);

const rv_parsed *rv_parser_result(const rv_parser *p);

#endif /* RV_PARSER_H */
