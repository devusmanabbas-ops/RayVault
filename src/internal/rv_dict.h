#ifndef RV_DICT_H
#define RV_DICT_H

#include "rv_section.h"

/*
 * Name dictionary helpers layered on the decoded SNAM table.
 * Lookups return borrowed pointers into the arena-backed strings.
 */

const rv_name_entry *rv_dict_by_id(const rv_snam_table *t, uint32_t id);
const rv_name_entry *rv_dict_by_str(const rv_snam_table *t, const char *s,
                                    size_t n);
int rv_dict_resolve(const rv_snam_table *t, uint32_t id, const char **out,
                    size_t *out_len);

/* Validate that all name ids referenced by a route table exist. */
int rv_dict_check_routes(const rv_snam_table *t, const rv_rout_table *r,
                         uint32_t *bad_id);
int rv_dict_check_instruments(const rv_snam_table *t, const rv_inst_table *r,
                              uint32_t *bad_id);
int rv_dict_check_markers(const rv_snam_table *t, const rv_mark_table *m,
                          uint32_t *bad_id);

#endif /* RV_DICT_H */
