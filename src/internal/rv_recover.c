#include "rv_recover.h"
#include "rv_dict.h"
#include "rv_normalize.h"

#include <string.h>

void rv_repair_options_default(rv_repair_options *o)
{
    if (!o)
        return;
    memset(o, 0, sizeof(*o));
    o->rebuild_summ = 1;
    o->drop_bad_links = 1;
    o->clamp_ranges = 1;
    o->synthesize_missing_names = 0;
}

int rv_repair_package(rv_reader *r, rv_parsed *p, rv_arena *a,
                      const rv_repair_options *o, rv_diag *diag)
{
    rv_repair_options def;
    rv_normalize_stats ns;
    uint32_t i, w;

    if (!r || !p || !a)
        return -1;
    if (!o) {
        rv_repair_options_default(&def);
        o = &def;
    }

    if (o->clamp_ranges)
        (void)rv_normalize_parsed(p, &ns);

    if (o->drop_bad_links && p->link.count) {
        /* Compact link table in place, dropping dangling marker refs. */
        w = 0;
        for (i = 0; i < p->link.count; i++) {
            int keep = 1;
            if (p->link.items[i].from_kind == 3) {
                uint32_t j, found = 0;
                for (j = 0; j < p->mark.count; j++) {
                    if (p->mark.items[j].marker_id == p->link.items[i].from_id) {
                        found = 1;
                        break;
                    }
                }
                if (!found)
                    keep = 0;
            }
            if (keep) {
                if (w != i)
                    p->link.items[w] = p->link.items[i];
                w++;
            }
        }
        p->link.count = w;
    }

    if (o->rebuild_summ) {
        p->summ.route_count = p->rout.count;
        p->summ.window_count = p->wind.count;
        p->summ.marker_count = p->mark.count;
        p->summ.wave_count = p->wave.count;
        p->summ.name_count = p->snam.count;
        p->summ.payload_bytes = r->backing.size;
        p->summ.flags |= 0x1; /* repaired */
        p->has_summ = 1;
    }

    if (diag)
        rv_diag_add(diag, RV_OK, "repair completed");
    return 0;
}
